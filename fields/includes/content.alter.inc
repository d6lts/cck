<?php

/**
 * Content Schema Alter
 *
 * Alter the database schema.
 *
 * TODO figure out an API-safe way to use batching to update the nodes that
 * will be affected by this change so the node_save() hooks will fire.
 *
 */
function content_alter_schema($previous = NULL, $new = NULL) {
  content_alter_db($previous, $new);
}

/**
 * Perform adds, alters, and drops as needed to synchronize the database with
 * new field definitions.
 *
 * XXX Document is = NULL nonsense.
 */
function content_alter_db($previous = NULL, $new = NULL) {
  $ret = array();

  // One or the other of these must be valid, but not both.  In fact,
  // this function should probably be split in to create and delete
  // functions.
  if (((!empty($previous) ? 1 : 0) + (!empty($new) ? 1 : 0)) != 1) {
    return $ret;
  }

  if (!empty($previous)) {
    $previous_field = field_instance_get_field($previous);
  }
  if (!empty($new)) {
    $new_field = field_instance_get_field($new);
  }

  // Gather relevant information : schema, table name...
  $previous_schema = !empty($previous_field) ? content_table_schema($previous_field) : array();
  $new_schema = !empty($new_field) ? content_table_schema($new_field) : array();
  if (!empty($previous_field)) {
    $previous_db_info = content_database_info($previous);
    $previous_table = $previous_db_info['table'];
  }
  if (!empty($new_field)) {
    $new_db_info = content_database_info($new);
    $new_table = $new_db_info['table'];
  }

  // Deletion of a field instance: drop relevant columns and tables and return.
  if (empty($new_field)) {
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
      db_drop_table($ret, $previous_table);
    }
    else {
      foreach ($previous_schema['fields'] as $column => $attributes) {
        if (!in_array($column, array('nid', 'vid', 'delta'))) {
          db_drop_field($ret, $previous_table, $column);
        }
      }
    }
    content_alter_db_cleanup();
    return $ret;
  }

  // Check that content types that have fields do have a per-type table.
  if (!empty($new_field)) {
    $base_tablename = _content_tablename($new['type_name'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    if (!db_table_exists($base_tablename)) {
      db_create_table($ret, $base_tablename, content_table_schema());
    }
  }

  // Create new table and columns, if not already created.
  if (!db_table_exists($new_table)) {
    db_create_table($ret, $new_table, $new_schema);
  }
  else {
    // Or add fields to an existing table.
    foreach ($new_schema['fields'] as $column => $attributes) {
      if (!in_array($column, array('nid', 'vid', 'delta')) && !db_column_exists($new_table, $column)) {
        db_add_field($ret, $new_table, $column, $attributes);
      }
    }
  }

  // If this is a new field, we're done.
  if (empty($previous_field)) {
    content_alter_db_cleanup();
    return $ret;
  }

  // If the previous table doesn't exist, we're done.
  // Could happen if someone tries to run a schema update from an
  // content.install update function more than once.
  if (!db_table_exists($previous_table)) {
    content_alter_db_cleanup();
    return $ret;
  }

  //////////////////////////////////////////////////////////////////////
  // THE END.  Everything below here is now called a field conversion
  // and will be handled separately.
  //////////////////////////////////////////////////////////////////////
  return;

  // If changing data from one schema to another, see if changes require that
  // we drop multiple values or migrate data from one storage type to another.
  $migrate_columns = array_intersect_assoc($new_schema['fields'], $previous_schema['fields']);
  unset($migrate_columns['nid'], $migrate_columns['vid'], $migrate_columns['delta']);

  // If we're going from one multiple value a smaller one or to single,
  // drop all delta values higher than the new maximum delta value.
  // Not needed if the new multiple is unlimited or if the new table is the content table.
  if ($new_table != $base_tablename && $new_field['multiple'] < $previous_field['multiple'] && $new_field['multiple'] != 1) {
    db_delete($new_table)
      ->condition('delta' , max(1, $new_field['multiple']), '>=')
      ->execute();
//    db_query("DELETE FROM {". $new_table ."} WHERE delta >= ". max(1, $new_field['multiple']));
  }

  // If going from multiple to non-multiple, make sure the field tables have
  // the right database structure to accept migrated data.
  if ($new_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD && count($previous_schema['fields'])) {
      // Already using per-field storage; change multiplicity if needed.
      if ($previous_field['multiple'] > 0 && $new_field['multiple'] == 0) {
        db_drop_field($ret, $new_table, 'delta');
        db_drop_primary_key($ret, $new_table);
        db_add_primary_key($ret, $new_table, array('vid'));
      }
      else if ($previous_field['multiple'] == 0 && $new_field['multiple'] > 0) {
        db_add_field($ret, $new_table, 'delta', array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'default' => 0));
        db_drop_primary_key($ret, $new_table);
        db_add_primary_key($ret, $new_table, array('vid', 'delta'));
      }
    }
  }

  // Migrate data from per-content-type storage.
  if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE &&
  $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
    $columns = array_keys($migrate_columns);
    if ($new_field['multiple']) {
      // TODO D7 : INSERT / SELECT ?
      db_query('INSERT INTO {'. $new_table .'} (vid, nid, delta, '. implode(', ', $columns) .') '.
        ' SELECT vid, nid, 0, '. implode(', ', $columns) .' FROM {'. $previous_table .'}');
    }
    else {
      // TODO D7 : INSERT / SELECT ?
      db_query('INSERT INTO {'. $new_table .'} (vid, nid, '. implode(', ', $columns) .') '.
        ' SELECT vid, nid, '. implode(', ', $columns) .' FROM {'. $previous_table .'}');
    }
    foreach ($columns as $column_name) {
      db_drop_field($ret, $previous_table, $column_name);
    }
  }

  // Migrate data from per-field storage, and drop per-field table.
  if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD &&
  $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
    // In order to be able to use drupal_write_record, we need to
    // rebuild the schema now.
    content_alter_db_cleanup();
    $query = db_select($previous_table, 'field', PDO::FETCH_ASSOC)
      ->join('node', 'n', 'field.nid = n.nid');
    $query->fields('field');
    $query->condition('n.type', $new_field['type_name']);
    if ($previous_field['multiple']) {
      $query->condition('delta', 0);
    }
    $resuls = $query->execute();

    $record = array();
    foreach ($results as $data) {
      $record['nid'] = $data['nid'];
      $record['vid'] = $data['vid'];
      if ($previous_field['multiple']) {
        $record['delta'] = $data['delta'];
      }
      foreach ($migrate_columns as $column => $attributes) {
        $record[$column] = is_null($data[$column]) ? NULL : $data[$column];
      }
      // TODO D7 : merge query ?
      if (db_query('SELECT COUNT(*) FROM {'. $new_table . '} WHERE vid = :vid AND nid = :nid', array(':vid' => $data['vid'], ':nid' => $data['nid']))->fetchField()) {
        $keys = $new_field['multiple'] ? array('vid', 'delta') : array('vid');
        drupal_write_record($new_table, $record, $keys);
      }
      else {
        drupal_write_record($new_table, $record);
      }
    }
    db_drop_table($ret, $previous_table);
  }

  // Change modified columns that don't involve storage changes.
  foreach ($new_schema['fields'] as $column => $attributes) {
    if (isset($previous_schema['fields'][$column]) &&
    $previous_field['db_storage'] == $new_field['db_storage']) {
      if ($attributes != $previous_schema['fields'][$column]) {
        if (!in_array($column, array('nid', 'vid', 'delta'))) {
          db_change_field($ret, $new_table, $column, $column, $attributes);
        }
      }
    }
  }

  // Remove obsolete columns.
  foreach ($previous_schema['fields'] as $column => $attributes) {
    if (!isset($new_schema['fields'][$column])) {
      if (!in_array($column, array('nid', 'vid', 'delta'))) {
        db_drop_field($ret, $previous_table, $column);
      }
    }
  }

  return $ret;
}

/**
 * Helper function for handling cleanup operations when schema changes are made.
 */
function content_alter_db_cleanup() {
  // Rebuild the whole database schema.
  // TODO: this could be optimized. We don't need to rebuild in *every case*...
  // Or do we? This affects the schema and menu and may have unfortunate
  // delayed effects if we don't clear everything out at this point.
  content_clear_type_cache(TRUE);
}
