<?php
// $Id$

/**
 * @file
 * Create/Read/Update/Delete functions for CCK-defined object types.
 */

/**
 * Create a new content type.
 *
 * @param $properties
 *   An array of properties to load the content type with. Valid keys:
 *   - '' - 
 * @return
 *   The ID of the newly-created content type.
 */
function content_type_create($properties) {
  // TODO
}

/**
 * Load a content type.
 *
 * @param $properties
 *   An array of properties to use in selecting a content type. Valid keys:
 *   - '' - 
 * @return
 *   The content type array.
 */
function content_type_read($properties) {
  // TODO
}

/**
 * Update an existing content type.
 *
 * @param $properties
 *   An array of properties to set in the content type. Valid keys:
 *   - '' - 
 * @return
 *   The number of content types updated.
 */
function content_type_update($properties) {
  // TODO
}

/**
 * Delete an existing content type.
 *
 * @param $properties
 *   An array of properties to use in selecting a content type. Valid keys:
 *   - 'type_name' - The name of the content type to be deleted.
 * @return
 *   The number of content types deleted.
 */
function content_type_delete($properties) {
  // Delete all nodes of this content type.
  $result = db_query("SELECT nid FROM {node} WHERE type = '%s'", $properties['type_name']);
  while ($node = db_fetch_object($result)) {
    node_delete($node->nid);
  }
  
  $result = db_query("SELECT field_name FROM {node_field_instance} WHERE type_name = '%s'", $properties['type_name']);
  $field_names = array();
  while ($type = db_fetch_array($result)) {
    $field_names[] = $type['field_name'];
  }
  foreach ($field_names as $field_name) {
    content_field_instance_delete(array('type_name' => $properties['type_name'], 'field_name' => $field_name));
  }

  $number_deleted = db_query("DELETE FROM {node_type_content} WHERE type_name = '%s'", $properties['type_name']);

  db_query("DROP TABLE {node_". $properties['type_name'] ."}");

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
  else if ($instances == 0) {
    if (is_array($columns) && count($columns)) {
      content_alter_db_field($field, $columns, array(), array());
    }
    db_query("DELETE FROM {node_field} WHERE field_name = '%s'", $properties['field_name']);
  }

  content_clear_type_cache();
  
  return $number_deleted;
}
