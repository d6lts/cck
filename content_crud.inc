<?php
// $Id$

/**
 * @file
 * Create/Read/Update/Delete functions for CCK-defined object types.
 */

/**
 * Create a new field.
 *
 * @param $field
 *   An array of properties to create the field with.
 */
function content_field_create($field) {
  $field = content_field_sanitize($field);
  $field['columns'] = (array) module_invoke($field['module'], 'field_settings', 'database columns', $field);

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'create', $field);

  // Write record and immediately create at least one field instance.
  _content_field_write($field, 'create');
  content_field_instance_create($field);

  content_clear_type_cache();

  // New fields require a menu rebuild to create all the field paths.
  menu_rebuild();

  // Create new database columns.
  if (is_array($field['columns']) && count($field['columns'])) {
    content_alter_db_field(array(), array(), $field, $field['columns']);
  }
}

/**
 * Update an existing field.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_update($field) {
  $field = content_field_sanitize($field);

  // Changes in field values may affect columns.
  $field['columns'] = (array) module_invoke($field['module'], 'field_settings', 'database columns', $field);

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'update', $field);

  _content_field_write($field, 'update');
  content_clear_type_cache();
}

/**
 * Make sure all field values are set.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_sanitize($field) {
  // Use schema to be sure all field values are set.
  $schema = drupal_get_schema(content_field_tablename());
  foreach ($schema['fields'] as $name => $info) {
    $field[$name] = !empty($field[$name]) ? $field[$name] : $info['default'];
  }
  // Make sure columns are formatted into an array.
  if (!is_array($field['columns'])) {
    $field['columns'] = (array) (!empty($field['columns']) ? unserialize($field['columns']) : array());
    if (is_array($field['columns'])) {
      // Ensure columns always default to NULL values.
      foreach ($field['columns'] as $column_name => $column) {
        $field['columns'][$column_name]['not null'] = FALSE;
      }
    }
  }
  // Make sure some value has been created for all global settings.
  $field_settings = (array) (!empty($field['global_settings']) ? unserialize($field['global_settings']) : array());
  $setting_names = module_invoke($field['module'], 'field_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field[$setting] = isset($field[$setting]) ? $field[$setting] : (isset($field_settings[$setting]) ? $field_settings[$setting] : '');
    }
  }
  unset($field['global_settings']);
  return $field;
}

/**
 * Write a field record.
 *
 * @param $field
 *   The field array to process.
 */
function _content_field_write($field, $op = 'update') {
  // Rearrange the data to create the global_settings array.
  $field_settings = array();
  $setting_names = module_invoke($field['module'], 'field_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field_settings[$setting] = isset($field[$setting]) ? $field[$setting] : '';
      unset($field[$setting]);
    }
  }
  $field['global_settings'] = serialize($field_settings);
  $field['columns'] = serialize($field['columns']);
  switch ($op) {
    case 'create':
      drupal_write_record(content_field_tablename(), $field);
      break;
    case 'update':
      drupal_write_record(content_field_tablename(), $field, 'field_name');
      break;
  }
}

/**
 * Load a field.
 *
 * @param $param
 *   An array of properties to use in selecting a field. Valid keys:
 *   field_name
 * @return
 *   The field array.
 */
function content_field_read($param) {
  $field = db_fetch_array(db_query("SELECT * FROM {". content_field_tablename() ."} nf WHERE field_name = '%s'", $param['field_name']));
  $field = content_field_sanitize($field);

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'read', $field);
  return $field;
}

/**
 * Delete an existing field.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_delete($field) {
  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'delete', $field);

  $fields[] = content_field_instance_read(array('field_name' => $field['field_name']));
  foreach ($fields as $field) {
    content_field_instance_delete($field);
  }
  // Deleted fields require a menu rebuild to remove all the field paths.
  menu_rebuild();
}


/**
 * Create a new field instance.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_instance_create($field) {
  $prior_instances = content_field_instance_read(array('field_name' => $field['field_name']));
  if ($prior_instances) {
    $field = array_merge(array_pop($prior_instances), $field);
  }
  $field = content_field_instance_sanitize($field);

  $field['widget_settings']  = !empty($field['widget_settings']) ? unserialize($field['widget_settings']) : array();
  $field['display_settings'] = !empty($field['display_settings']) ? unserialize($field['display_settings']) : array();

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'create instance', $field);

  if (is_array($field['columns']) && count($field['columns'])) {
    if ($field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
      $new_field = $field;
      $new_field['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;
      drupal_write_record(content_field_tablename(), $new_field, 'field_name');
      content_alter_db_field($field, $field['columns'], $new_field, $field['columns']);
    }
  }

  _content_field_instance_write($field, 'create');
  content_clear_type_cache();
}

/**
 * Update an existing field instance.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_instance_update($field) {
  // If content.module is handling the default value,
  // initialize $widget_settings with default values,
  if (content_callback('widget', 'default value', $field) == CONTENT_CALLBACK_DEFAULT) {
    $widget_settings = array(
      'default_value' => $field['default_value'],
      'default_value_php' => $field['default_value_php'],
    );
  }

  $prev_field = content_field_instance_sanitize(content_field_sanitize($field));
  $prev_columns = $field['columns'];

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'update instance', $field);

  _content_field_instance_write($field, 'update');

  if ($field['multiple']) {
    $field['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;
  }
  else {
    $instances = content_field_instance_read(array('field_name' => $field['field_name']));
    if (sizeof($instances) == 1) {
      $field['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
    }
  }
  content_field_update($field);

  content_clear_type_cache();

  $new_field = content_fields($field['field_name']);
  $new_columns = $new_field['columns'];

  if (!isset($prev_columns)) {
    $prev_columns = array();
  }
  if (!isset($new_columns)) {
    $new_columns = array();
  }
  content_alter_db_field($prev_field, $prev_columns, $new_field, $new_columns);
}

/**
 * Make sure all field values are set.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_instance_sanitize($field) {
  // Use schema to be sure all field values are set.
  $schema = drupal_get_schema(content_instance_tablename());
  foreach ($schema['fields'] as $name => $info) {
    $field[$name] = !empty($field[$name]) ? $field[$name] : $info['default'];
  }
  // Make sure widget settings all have a value set in the array.
  $field_settings = (array) unserialize($field['widget_settings']);
  $setting_names = module_invoke($field['widget_module'], 'widget_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field[$setting] = isset($field[$setting]) ? $field[$setting] : (isset($field_settings[$setting]) ? $field_settings[$setting] : '');
    }
  }
  unset($field['widget_settings']);
  $field['label'] = !empty($field['label']) ? $field['label'] : $field['field_name'];
  return $field;
}

/**
 * Write a field instance record.
 *
 * @param $field
 *   The field array to process.
 */
function _content_field_instance_write($field, $op = 'update') {
  // Rearrange the data to create the widget_settings array.
  $field_settings = array();
  $setting_names = module_invoke($field['widget_module'], 'widget_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $name => $setting) {
      $field_settings[$setting] = isset($field[$setting]) ? $field[$setting] : '';
      unset($field[$name]);
    }
  }
  $field['widget_settings'] = serialize($field_settings);
  $field['display_settings'] = serialize($field['display_settings']);
  switch ($op) {
    case 'create':
      drupal_write_record(content_instance_tablename(), $field);
      break;
    case 'update':
      drupal_write_record(content_instance_tablename(), $field, 'field_name');
      break;
  }
}

/**
 * Load a field instance.
 *
 * @param $param
 *   An array of properties to use in selecting a field instance. Valid keys:
 *   - 'type_name' - The name of the content type in which the instance exists.
 *   - 'field_name' - The name of the field whose instance is to be loaded.
 *   if NULL, all instances will be returned.
 * @return
 *   The field arrays.
 */
function content_field_instance_read($param = NULL) {
  if (is_array($param)) {
    // Turn the conditions into a query.
    foreach ($param as $key => $value) {
      $cond[] = 'nfi.'. db_escape_string($key) ." = '%s'";
      $args[] = $value;
    }
  }
  if (count($cond)) {
    $cond = " WHERE ". implode(' AND ', $cond);
  }
  $db_result = db_query("SELECT * FROM {". content_instance_tablename() ."} nfi ". $cond ." ORDER BY nfi.weight ASC, nfi.label ASC", $args);
  $fields = array();
  while ($field = db_fetch_array($db_result)) {
    $field = content_field_instance_sanitize($field);
    $field = _content_field_instance_expand(array_merge(content_field_read($field), $field));
    // Invoke hook_content_fieldapi().
    module_invoke_all('content_fieldapi', 'read instance', $field);
    $fields[] = $field;
  }
  return $fields;
}

/**
 * Expand field info to create $field['widget'] info.
 *
 * Private function to expand a field.
 */
function _content_field_instance_expand($field) {
  // Overwrite global field information with specific information
  $widget_settings = $field['widget_settings'] ? unserialize($field['widget_settings']) : array();
  unset($field['widget_settings']);
  $field['widget'] = $widget_settings;
  $field['widget']['type'] = $field['widget_type'];
  unset($field['widget_type']);
  $field['widget']['weight'] = $field['weight'];
  unset($field['weight']);
  $field['widget']['label'] = $field['label'];
  unset($field['label']);
  $field['widget']['description'] = $field['description'];
  unset($field['description']);
  $field['display_settings'] = $field['display_settings'] ? unserialize($field['display_settings']) : array();
  return $field;
}

/**
 * Delete an existing field instance.
 *
 * @param $field
 *   The field array to process.
 */
function content_field_instance_delete($field) {
  $field = content_field_instance_sanitize(content_field_sanitize($field));

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'delete instance', $field);

  db_query("DELETE FROM {". content_instance_tablename() ."} WHERE field_name = '%s' AND type_name = '%s'", $field['field_name'], $field['type_name']);

  $instances = content_field_instance_read(array('field_name' => $field['field_name']));

  // If only one instance remains, we may need to change the database
  // representation for this field.
  if (sizeof($instances) == 1) {
    if (!($field['multiple'])) {
      // Multiple-valued fields are always stored per-content-type.
      if (is_array($field['columns']) && count($field['columns'])) {
        $new_field = $field;
        $new_field['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
        content_field_update($new_field);
        content_alter_db_field($field, $field['columns'], $new_field, $field['columns']);
      }
    }
  }

  // If no instances remain, delete the field entirely.
  elseif (sizeof($instances) == 0) {
    if (is_array($columns) && count($columns)) {
      content_alter_db_field($field, $columns, array(), array());
    }
    db_query("DELETE FROM {". content_field_tablename() ."} WHERE field_name = '%s'", $field_name);
  }
}

/**
 * Delete all data related to a module.
 *
 * @param string $module
 */
function content_module_delete($module) {
  // Delete the field data.
  $results = db_query("SELECT * FROM {". content_field_tablename() ."} WHERE module = '%s'", $module);
  while ($field = db_fetch_array($results)) {
    $field['columns'] = unserialize($field['columns']);
    content_field_delete($field);
  }
  // Delete the field settings.
  db_query("DELETE FROM {". content_field_tablename() ."} WHERE module = '%s'", $module);
  db_query("DELETE FROM {". content_instance_tablename() ."} WHERE widget_module = '%s'", $module);
  // Force the caches and static arrays to update to the new info.
  _content_type_info(TRUE, TRUE);

}

/**
 * Rebuild content type information from node tables.
 *
 *  Used to update CCK tables that might have missed changes made when CCK was disabled.
 *  Called by hook_form_alter() on system modules page whenever CCK is enabled.
 */
function content_types_rebuild() {
  $db_types = content_types();

  $result = db_query("SELECT type_name FROM {". content_instance_tablename() ."}");
  while ($type = db_fetch_array($result)) {
    $field_types[] = $type['type_name'];
  }

  foreach ($db_types as $content_type) {
    // Find content types that are missing the content table and add it
    content_type_create((object)array('type' => $content_type['type']));
  }
  content_clear_type_cache();
}

/**
 * Make changes needed when a content type is created.
 *
 * @param $info
 *   value supplied by hook_node_type()
 */
function content_type_create($info) {
  content_clear_type_cache();
  $type = content_types($info->type);
  $table = _content_tablename($type['type'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
  if (!db_table_exists($table)) {
    $ret = array();
    db_create_table($ret, $table, _content_table_schema());
    drupal_set_message(t('The content fields table %name has been created.', array('%name' => $table)));
  }
}

/**
 * Make changes needed when an existing content type is updated.
 *
 * @param $info
 *   value supplied by hook_node_type()
 */
function content_type_update($info) {
  if (!empty($info->old_type) && $info->old_type != $info->type) {
    // rename the content type in all fields that use changed content type.
    db_query("UPDATE {". content_instance_tablename() ."} SET type_name='%s' WHERE type_name='%s'", array($info->type, $info->old_type));

    // Rename the content fields table to match new content type name.
    $old_type = content_types($info->old_type);
    $old_name = _content_tablename($old_type['type'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    $new_name = _content_tablename($info->type, CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    if (db_table_exists($old_name)) {
      $ret = array();
      db_rename_table($ret, $old_name, $new_name);
      drupal_set_message(t('Content fields table %old_name has been renamed to %new_name and field instances have been updated.', array('%old_name' => $old_name, '%new_name' => $new_name)));
    }
  }
  // reset all content type info
  content_clear_type_cache();
}

/**
 * Make changes needed when a content type is deleted.
 *
 * @param $info
 *   value supplied by hook_node_type()
 */
function content_type_delete($info) {
  $type = content_types($info->type);
  foreach ($type['fields'] as $field) {
    content_field_instance_delete($field);
  }
  $table = _content_tablename($type['type'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
  if (db_table_exists($table)) {
    $ret = array();
    db_drop_table($ret, $table);
    drupal_set_message(t('The content fields table %name has been deleted.', array('%name' => $table)));
  }

  // reset all content type info
  content_clear_type_cache();
}