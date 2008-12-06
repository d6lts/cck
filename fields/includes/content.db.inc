<?php
// $Id$

/**
 * Helper function for identifying the storage type for a field.
 */
function content_storage_type($field) {
  if ($field['multiple'] > 0 || $field['shareable']) {
    return CONTENT_DB_STORAGE_PER_FIELD;
  }
  return CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
}

/**
 * A basic schema used by all field and type tables.
 *
 * This will only add the columns relevant for the specified field.
 * Leave $field['columns'] empty to get only the base schema,
 * otherwise the function will return the whole thing.
 */
function content_table_schema($field = NULL) {
  $schema = array(
    'fields' => array(
      'vid' => array('type' => 'int', 'unsigned' => TRUE, 'not null' => TRUE, 'default' => 0),
      'nid' => array('type' => 'int', 'unsigned' => TRUE, 'not null' => TRUE, 'default' => 0)
    ),
    'primary key' => array('vid'),
    'indexes' => array(
      'nid'    => array('nid'),
    ),
  );

  // Add delta column if needed.
  if (!empty($field['multiple'])) {
    $schema['fields']['delta'] = array('type' => 'int', 'unsigned' => TRUE, 'not null' => TRUE, 'default' => 0);
    $schema['primary key'][] = 'delta';
  }
  $schema['content fields'] = array();

  // Add field columns column if needed.
  // This function is called from install files where it is not safe
  // to use content_fields() or content_database_info(), so we
  // just used the column values stored in the $field.
  // We also need the schema to include fields from disabled modules
  // or there will be no way to delete those fields.

  if (!empty($field['columns'])) {
    foreach ($field['columns'] as $column => $attributes) {
      $column_name =  $field['field_name'] .'_'. $column;
      unset($attributes['column']);
      unset($attributes['sortable']);
      $schema['fields'][$column_name] = $attributes;
    }
    $schema['content fields'][] = $field['field_name'];
  }
  return $schema;
}

function content_storage_load($nodes) {
  // Group vids by type.
  $node_types = array();
  foreach ($nodes as $nid => $node) {
    $node_types[$node->type][] = $node->vid;
  }

  $additions = array();
  foreach ($node_types as $type_name => $vids) {
    $type = content_types($type_name);
    // OPTIMIZE: load all non multiple fields in a single JOIN query ?
    // warning: 61-join limit in MySQL ?
    foreach ($type['tables'] as $table) {
      // TODO D7 : we could store 'content fields' in type['tables'] instead of schema.
      $schema = drupal_get_schema($table);
      // The per-type table might not have any fields actually stored in it.
      // TODO D7 : remove this specific case (empty per-type table) ?
      if (!$schema['content fields']) {
        continue;
      }

      $query = db_select($table, 't')
        ->fields('t')
        ->condition('vid', $vids, 'IN');
      // If we're loading a table for a multiple field,
      // we fetch all rows (values) ordered by delta.
      if (isset($schema['fields']['delta'])) {
        $query->orderBy('delta');
      }
      $results = $query->execute();
      foreach ($results as $row) {
        // For each field stored in the table, add the field item.
        foreach ($schema['content fields'] as $field_name) {
          $item = array();
          $instance = content_fields($field_name, $type_name);
          $db_info = content_database_info($instance);
          // For each column declared by the field, populate the item.
          foreach ($db_info['columns'] as $column => $attributes) {
            $item[$column] = $row->$attributes['column'];
          }

          // Add the item to the field values for the node.
          if (!isset($additions[$row->nid][$field_name])) {
            $additions[$row->nid][$field_name] = array();
          }
          $additions[$row->nid][$field_name][] = $item;
        }
      }
    }
  }
  return $additions;
}

function content_storage_write($node, $update = FALSE) {
  $type_name = $node->type;
  $type = content_types($type_name);

  // TODO D7 : we used [content|drupal]_write_record mainly because it generated placeholders for us. Unneeded now with PDO.
  // !! Be sure to check direct node_save with missing fields (leave field as is) / fields set to NULL (set field to empty)
  foreach ($type['tables'] as $table) {
    $schema = drupal_get_schema($table);
    $record = array();
    foreach ($schema['content fields'] as $field_name) {
      if (isset($node->$field_name)) {
        $instance = content_fields($field_name, $type_name);

        // Multiple fields need specific handling, we'll deal with
        // them later on.
        $field = field_instance_get_field($instance);
        if ($field['multiple']) {
          continue;
        }

        $db_info = content_database_info($instance);
        foreach ($db_info['columns'] as $column => $attributes) {
          $record[$attributes['column']] = $node->{$field_name}[0][$column];
        }
      }
    }
    // $record might be empty because
    // - the table stores a multiple field :
    //   we do nothing, this is handled later on
    // - this is the per-type table and no field is actually stored in it :
    //   we still store the nid and vid
    // TODO D7 : remove this specific case (empty per-type table) ?
    if (count($record) || empty($schema['content fields'])) {
      $record['nid'] = $node->nid;
      $record['vid'] = $node->vid;
      // Can't rely on the insert/update op of the node to decide if this
      // is an insert or an update, a node or revision may have existed
      // before any fields were created, so there may not be an entry here.

      // TODO - should we auto create an entry for all existing nodes when
      // fields are added to content types -- either a NULL value
      // or the default value? May need to offer the user an option of
      // how to handle that.

      // TODO D7 : merge query ?
      if (db_query("SELECT COUNT(*) FROM {". $table ."} WHERE vid = :vid", array(':vid' => $node->vid))->fetchField()) {
        content_write_record($table, $record, array('vid'));
      }
      else {
        content_write_record($table, $record);
      }
    }
  }

  // Handle multiple fields.
  foreach ($type['instances'] as $instance) {
    $field = field_instance_get_field($instance);
    $field_name = $field['field_name'];
    if ($field['multiple'] && isset($node->$field_name)) {
      $db_info = content_database_info($instance);
      // TODO D7 : Merge query ? multi update ?
      // Delete and insert, rather than update, in case a value was added.
      if ($update) {
        db_delete($db_info['table'])
          ->condition('vid', $node->vid)
          ->execute();
      }
      foreach ($node->$field_name as $delta => $item) {
        $record = array();
        foreach ($db_info['columns'] as $column => $attributes) {
          $record[$attributes['column']] = $item[$column];
        }
        $record['nid'] = $node->nid;
        $record['vid'] = $node->vid;
        $record['delta'] = $delta;
        content_write_record($db_info['table'], $record);
      }
    }
  }
}

function content_storage_delete($node) {
  $type_name = $node->type;
  $type = content_types($type_name);
  foreach ($type['tables'] as $table) {
    db_delete($table)
      ->condition('nid', $node->nid)
      ->execute();
  }
}

function content_storage_delete_revision($node) {
  $type_name = $node->type;
  $type = content_types($type_name);
  foreach ($type['tables'] as $table) {
    db_delete($table)
      ->condition('vid', $node->vid)
      ->execute();
  }
}

/**
 * Save a record to the database based upon the schema.
 *
 * Directly copied from core's drupal_write_record, which can't update a
 * column to NULL. See http://drupal.org/node/227677 and
 * http://drupal.org/node/226264 for more details about that problem.
 *
 * Default values are filled in for missing items, and 'serial' (auto increment)
 * types are filled in with IDs.
 *
 * @param $table
 *   The name of the table; this must exist in schema API.
 * @param $object
 *   The object to write. This is a reference, as defaults according to
 *   the schema may be filled in on the object, as well as ID on the serial
 *   type(s). Both array an object types may be passed.
 * @param $update
 *   If this is an update, specify the primary keys' field names. It is the
 *   caller's responsibility to know if a record for this object already
 *   exists in the database. If there is only 1 key, you may pass a simple string.
 * @return
 *   Failure to write a record will return FALSE. Otherwise SAVED_NEW or
 *   SAVED_UPDATED is returned depending on the operation performed. The
 *   $object parameter contains values for any serial fields defined by
 *   the $table. For example, $object->nid will be populated after inserting
 *   a new node.
 */
function content_write_record($table, &$object, $update = array()) {
  // Standardize $update to an array.
  if (is_string($update)) {
    $update = array($update);
  }

  // Convert to an object if needed.
  if (is_array($object)) {
    $object = (object) $object;
    $array = TRUE;
  }
  else {
    $array = FALSE;
  }

  $schema = drupal_get_schema($table);
  if (empty($schema)) {
    return FALSE;
  }

  $fields = $defs = $values = $serials = $placeholders = array();

  // Go through our schema, build SQL, and when inserting, fill in defaults for
  // fields that are not set.
  foreach ($schema['fields'] as $field => $info) {
    // Special case -- skip serial types if we are updating.
    if ($info['type'] == 'serial' && count($update)) {
      continue;
    }

    // For inserts, populate defaults from Schema if not already provided
    if (!isset($object->$field) && !count($update) && isset($info['default'])) {
      $object->$field = $info['default'];
    }

    // Track serial fields so we can helpfully populate them after the query.
    if ($info['type'] == 'serial') {
      $serials[] = $field;
      // Ignore values for serials when inserting data. Unsupported.
      unset($object->$field);
    }

    // Build arrays for the fields, placeholders, and values in our query.
    if (isset($object->$field) || array_key_exists($field, $object)) {
      $fields[] = $field;
      if (isset($object->$field)) {
        $placeholders[] = db_type_placeholder($info['type']);

        if (empty($info['serialize'])) {
          $values[] = $object->$field;
        }
        else {
          $values[] = serialize($object->$field);
        }
      }
      else {
        $placeholders[] = 'NULL';
      }
    }
  }

  // Build the SQL.
  $query = '';
  if (!count($update)) {
    $query = "INSERT INTO {". $table ."} (". implode(', ', $fields) .') VALUES ('. implode(', ', $placeholders) .')';
    $return = SAVED_NEW;
  }
  else {
    $query = '';
    foreach ($fields as $id => $field) {
      if ($query) {
        $query .= ', ';
      }
      $query .= $field .' = '. $placeholders[$id];
    }

    foreach ($update as $key) {
      $conditions[] = "$key = ". db_type_placeholder($schema['fields'][$key]['type']);
      $values[] = $object->$key;
    }

    $query = "UPDATE {". $table ."} SET $query WHERE ". implode(' AND ', $conditions);
    $return = SAVED_UPDATED;
  }

  // Execute the SQL.
  if (db_query($query, $values)) {
    if ($serials) {
      // Get last insert ids and fill them in.
      foreach ($serials as $field) {
        $object->$field = db_last_insert_id($table, $field);
      }
    }

    // If we began with an array, convert back so we don't surprise the caller.
    if ($array) {
      $object = (array) $object;
    }

    return $return;
  }

  return FALSE;
}

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
