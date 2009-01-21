<?php

/**
 * Generate a table name for a bundle data table.
 *
 * @param $name
 *   The name of the bundle
 * @return
 *   A string containing the generated name for the database table
 */
function pbs_tablename($name) {
  return 'field_bundle_' . $name;
}

/**
 * Generate a table name for a bundle revision archive table.
 *
 * @param $name
 *   The name of the bundle
 * @return
 *   A string containing the generated name for the database table
 */
function pbs_revision_tablename($name) {
  return 'field_bundle_revision_' . $name;
}

/**
 * Return the database schema for a bundle table and its corresponding
 * revision table.  A bundle table has columns to store every
 * non-unlimited field's data in the bundle.
 *
 * @param $bundle
 *   The bundle name for which to create table schemas.
 * @return
 *   One or more tables representing the schema for the bundle tables.
 */
function pbs_bundle_schema($bundle) {
  $current = array(
    'description' => 'Bundle table for bundle '. $bundle,
    'fields' => array(
      'entity' => array(
        'type' => 'varchar',
        'length' => 255,
        'not null' => TRUE,
        'description' => 'The entity type this data is attached to',
      ),
      'entity_id' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => TRUE,
        'description' => 'The entity id this data is attached to',
      ),
      'revision_id' => array(
        'type' => 'int',
        'unsigned' => TRUE,
        'not null' => FALSE,
        'description' => 'The entity revision id this data is attached to, or NULL if the entity type is not versioned',
      ),
    ),
    'primary key' => array('entity', 'entity_id'),
  );

  $instances = field_info_instances($bundle);
  foreach ($instances as $instance) {
    $field = field_info_field($instance['field_name']);
    $columns = (array) module_invoke($field['module'], 'field_columns', $field);
    if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED) {
      for ($i = 0; $i < $field['cardinality']; ++$i) {
        foreach ($columns as $name => $spec) {
          $column_name = $field['field_name'] .'_'. $name .'_'. $i;
          $current['fields'][$column_name] = $spec;
        }
      }
    }
  }

  // Construct the revision table.  The primary key includes
  // revision_id but not entity_id so that multiple revision loads can
  // use the IN operator.
  $revision = $current;
  $revision['description'] = 'Revision archive for bundle table for bundle '. $bundle;
  $revision['revision_id']['description'] = 'The entity revision id this data is attached to';
  $revision['primary key'] = array('entity', 'revision_id');

  return array(
    pbs_tablename($bundle) => $current,
    pbs_revision_tablename($bundle) => $revision,
  );
}

/**
 * Create the per-bundle storage table for a bundle, dropping them
 * first if they already exist.
 *
 * @param $bundle
 *   The name of the bundle.
 */
function pbs_recreate_bundle_tables($bundle) {
  $schema = pbs_bundle_schema($bundle);
  foreach ($schema as $name => $table) {
    if (db_table_exists($name)) {
      db_drop_table($ret, $name);
    }
    db_create_table($ret, $name, $table);
  }
}

/**
 * Synchronize a single bundle table from its constituent field
 * tables.
 *
 * TODO: Synchronize revisions.
 *
 * @param $bundle
 *   The bundle to synchronize.
 */
function pbs_synchronize_bundle($bundle) {
  $bundle_table = pbs_tablename($bundle);

  $instances = field_info_instances($bundle);
  foreach ($instances as $instance) {
    $field = field_info_field($instance['field_name']);
    $field_table = _field_sql_storage_tablename($field['field_name']);
    if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED) {
      $query = db_select($field_table, 't', array('fetch' => PDO::FETCH_ASSOC))->fields('t');
      $results = $query->condition('bundle', $bundle)->execute();
      
      $pkey = array('entity' => 0, 'id' => 1);
      foreach ($results as $row) {
        $delta = $row['delta'];
        unset($row['bundle'], $row['delta'], $row['deleted']);
        $keys = array_intersect_key($row, $pkey);
        foreach ($field['columns'] as $name => $spec) {
          $column_name = $field['field_name'] .'_'. $name;
          $row[$column_name .'_'. $delta] = $row[$column_name];
          unset($row[$column_name]);
        }
        $query = db_merge($bundle_table)->fields($row)->key($keys);
        $query->execute();
      }
    }
  }
}

/**
 * Implementation of field_attach_update.
 *
 * When an object's fields are saved, update its bundle table.
 */
function pbs_field_attach_update($obj_type, $object) {
  list($id, $vid, $bundle, $cacheable) = field_attach_extract_ids($obj_type, $object);
  $bundle_table = pbs_tablename($bundle);
  $revision_table = pbs_revision_tablename($bundle);

  db_delete($bundle_table)->condition('entity', $obj_type)->condition('entity_id', $id)->execute();
  if (isset($vid)) {
    db_delete($revision_table)->condition('entity', $obj_type)->condition('revision_id', $vid)->execute();
  }

  $row = array('entity' => $obj_type, 'entity_id' => $id, 'revision_id' => $vid);

  $instances = field_info_instances($bundle);
  foreach ($instances as $instance) {
    $field = field_info_field($instance['field_name']);
    if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED) {
      for ($delta = 0; $delta < $field['cardinality']; ++$delta) {
        foreach ($field['columns'] as $name => $spec) {
          $row[$spec['column'] .'_'. $delta] = $object->{$field['field_name']}[$delta][$name];
        }
      }
    }
  }
          
  db_insert($bundle_table)->fields($row)->execute();
  if (isset($vid)) {
    db_insert($revision_table)->fields($row)->execute();
  }
}

/**
 * Implementation of field_attach_insert.
 *
 * When an object's fields are saved, update its bundle table.
 */
function pbs_field_attach_insert($obj_type, $object) {
  pbs_field_attach_update($obj_type, $object);
}

/**
 * Implementation of field_attach_delete.
 *
 * When an object's fields are deleted, update its bundle table.
 */
function pbs_field_attach_delete($obj_type, $object) {
  list($id, $vid, $bundle, $cacheable) = field_attach_extract_ids($obj_type, $object);
  $bundle_table = pbs_tablename($bundle);
  db_delete($bundle_table)->condition('entity_id', $id)->execute();
}

/**
 * Implementation of hook_field_create_instance.
 *
 * When a field is added to a bundle, re-generate and synchronize the
 * bundle table.  NOTE: This could just add the new columns.
 */
function pbs_field_create_instance($instance) {
  pbs_recreate_bundle_tables($instance['bundle']);
  pbs_synchronize_bundle($instance['bundle']);
}

/**
 * Implementation of hook_field_delete_instance.
 *
 * When a field is removed from a bundle, re-generate and synchronize
 * the bundle table.  NOTE: This could just drop the old columns.
 */
function pbs_field_delete_instance($instance) {
  pbs_recreate_bundle_tables($instance['bundle']);
  pbs_synchronize_bundle($instance['bundle']);
}

/**
 * Implementation of hook_field_attach_create_bundle.
 *
 * Create the (empty) bundle tables.
 */
function pbs_field_attach_create_bundle($bundle) {
  pbs_recreate_bundle_tables($bundle);
}

/**
 * Implementation of hook_field_attach_rename_bundle.
 *
 * Rename the bundle tables.
 */
function pbs_field_attach_rename_bundle($old_bundle, $new_bundle) {
  db_rename_table($ret, pbs_tablename($old_bundle), pbs_tablename($new_bundle));
  db_rename_table($ret, pbs_revision_tablename($old_bundle), pbs_revision_tablename($new_bundle));
}

/**
 * Implementation of hook_field_attach_delete_bundle.
 *
 * Delete the bundle tables.
 */
function pbs_field_attach_delete_bundle($bundle) {
  db_drop_table($ret, pbs_tablename($bundle));
  db_drop_table($ret, pbs_revision_tablename($bundle));
}

