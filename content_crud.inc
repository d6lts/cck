<?php
// $Id$

/**
 * @file
 * Create/Read/Update/Delete functions for CCK-defined object types.
 */

/**
 * Make changes needed when a content type is created.
 *
 * @param $info
 *   value supplied by hook_node_type()
 * @return
 *   The ID of the newly-created content type.
 */
function content_type_create($info) {
  // TODO
  // reset all content type info
  content_clear_type_cache();

}

/**
 * Load a content type.
 *
 * @param $info
 *   value supplied by hook_node_type()
 * @return
 *   The content type array.
 */
function content_type_read($info) {
  // TODO
}

/**
 * Make changes needed when an existing content type is updated.
 *
 * @param $info
 *   value supplied by hook_node_type()
 * @return
 *   The number of content types updated.
 */
function content_type_update($info) {

  $updated = 0;
  if ($info->type != $info->old_type) {
    // update all fields that use changed content type
    db_query("UPDATE {node_field_instance} SET type_name='%s' WHERE type_name='%s'", array($info->type, $info->old_type));
    $updated = 1;
  }
  // reset all content type info
  content_clear_type_cache();

  return $updated;
}

/**
 * Make changes needed when a content type is deleted.
 *
 * @param $info
 *   value supplied by hook_node_type()
 * @return
 *   The number of fields deleted.
 */
function content_type_delete($info) {

  $result = db_query("SELECT field_name FROM {node_field_instance} WHERE type_name = '%s'", $info->type);
  $field_names = array();
  while ($type = db_fetch_array($result)) {
    $field_names[] = $type['field_name'];
  }
  $number_deleted = 0;
  foreach ($field_names as $field_name) {
    content_field_instance_delete(array('type_name' => $info->type, 'field_name' => $field_name));
    $number_deleted++;
  }

  if (db_table_exists("node_". $info->type)) {
    db_query("DROP TABLE {node_". $info->type ."}");
  }

  // reset all content type info
  content_clear_type_cache();

  return $number_deleted;
}


/**
 * Create a new field.
 *
 * Any call to this function *must* be immediately followed by a call to
 * content_field_instance_create(), or the database could be left in an
 * inconsistent state.
 *
 * @param $properties
 *   An array of properties to load the field with. Valid keys:
 *   - '' -
 * @return
 *   The ID of the newly-created field.
 */
function content_field_create($properties) {
  // TODO
}

/**
 * Load a field.
 *
 * @param $properties
 *   An array of properties to use in selecting a field. Valid keys:
 *   - '' -
 * @return
 *   The field array.
 */
function content_field_read($properties) {
  // TODO
}

/**
 * Update an existing field.
 *
 * @param $properties
 *   An array of properties to set in the field. Valid keys:
 *   - '' -
 * @return
 *   The number of fields updated.
 */
function content_field_update($properties) {
  // TODO
}

/**
 * Delete an existing field.
 *
 * @param $properties
 *   An array of properties to use in selecting a field. Valid keys:
 *   - 'field_name' - The name of the field to be deleted.
 * @return
 *   The number of fields deleted.
 */
function content_field_delete($properties) {
  $result = db_query("SELECT type_name FROM {node_field_instance} WHERE field_name = '%s'", $properties['field_name']);
  $type_names = array();
  while ($type = db_fetch_array($result)) {
    $type_names[] = $type['type_name'];
  }
  foreach ($type_names as $type_name) {
    content_field_instance_delete(array('type_name' => $type_name, 'field_name' => $properties['field_name']));
  }
  return (count($type_names) ? 1 : 0);
}


/**
 * Create a new field instance.
 *
 * @param $properties
 *   An array of properties to load the field instance with. Valid keys:
 *   - '' -
 * @return
 *   The ID of the newly-created field instance.
 */
function content_field_instance_create($properties) {
  // TODO
}

/**
 * Load a field instance.
 *
 * @param $properties
 *   An array of properties to use in selecting a field instance. Valid keys:
 *   - 'type_name' - The name of the content type in which the instance exists.
 *   - 'field_name' - The name of the field whose instance is to be loaded.
 * @return
 *   The field instance array.
 */
function content_field_instance_read($properties) {
  // TODO
}

/**
 * Update an existing field instance.
 *
 * @param $properties
 *   An array of properties to set in the field instance. Valid keys:
 *   - '' -
 * @return
 *   The number of field instance updated.
 */
function content_field_instance_update($properties) {
  // TODO
}

/**
 * Delete an existing field instance.
 *
 * @param $properties
 *   An array of properties to use in selecting a field instance. Valid keys:
 *   - 'type_name' - The name of the content type in which the instance exists.
 *   - 'field_name' - The name of the field whose instance is to be deleted.
 * @return
 *   The number of field instances deleted.
 */
function content_field_instance_delete($properties) {
  $number_deleted = db_query("DELETE FROM {node_field_instance} WHERE type_name = '%s' AND field_name = '%s'", $properties['type_name'], $properties['field_name']);

  $type = content_types($properties['type_name']);
  $field = $type['fields'][$properties['field_name']];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $columns = module_invoke($field_type['module'], 'field_settings', 'database columns', $field);

  $instances = db_result(db_query("SELECT COUNT(*) FROM {node_field_instance} WHERE field_name = '%s'", $properties['field_name']));

  // If only one instance remains, we may need to change the database
  // representation for this field.
  if ($instances == 1) {
    if (!($field['multiple'])) {
      // Multiple-valued fields are always stored per-content-type.
      if (is_array($columns) && count($columns)) {
        $new_field = $field;
        $new_field['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
        db_query("UPDATE {node_field} SET db_storage = %d WHERE field_name = '%s'", CONTENT_DB_STORAGE_PER_CONTENT_TYPE, $properties['field_name']);
        content_alter_db_field($field, $columns, $new_field, $columns);
      }
    }
  }

  // If no instances remain, delete the field entirely.
  elseif ($instances == 0) {
    if (is_array($columns) && count($columns)) {
      content_alter_db_field($field, $columns, array(), array());
    }
    db_query("DELETE FROM {node_field} WHERE field_name = '%s'", $properties['field_name']);
  }

  content_clear_type_cache();

  return $number_deleted;
}

/**
 * Implementation of hook_simpletest().
 */
function content_simpletest() {
	$dir = drupal_get_path('module', 'content'). '/tests';
	$tests = file_scan_directory($dir, '\.test$');
  return array_keys($tests);
}

