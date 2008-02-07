<?php
// $Id$

/**
 * @file
 * Create/Read/Update/Delete functions for CCK-defined object types.
 *
 * The content module field API will allow $field arguments to
 * be input either in the field => widget nested array that is used
 * by the content module, or in flattened $form_values arrays, by
 * converting flattened arrays to the nested format.
 *
 * A hook_content_fieldapi() is available for each field instance action,
 * and each hook receives the nested field => widget array as an argument.
 *
 * The hook_content_fieldapi() $ops include:
 *
 *   - create instance
 *   - read instance
 *   - update instance
 *   - delete instance
 *
 * Another function, content_module_delete($module) will clean up
 * after a module that has been deleted by removing all data and
 * settings information that was created by that module.
 */

/**
 * Create an array of default values for a field type.
 */
function content_field_default_values($field_type) {
  $field_types = _content_field_types();
  $module = $field_types[$field_type]['module'];

  $field = array(
    'module' => $module,
    'type' => $field_type,
    'active' => 0,
  );

  if (module_exists($module)) {
    $field['active'] = 1;
  }

  $field['columns'] = module_invoke($module, 'field_settings', 'database columns', $field);
  // Ensure columns always default to NULL values.
  foreach ((array) $field['columns'] as $column_name => $column) {
    $field['columns'][$column_name]['not null'] = FALSE;
  }

  $field['required'] = 0;
  $field['multiple'] = 0;
  $field['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;

  // Make sure field settings all have an index in the array.
  $setting_names = module_invoke($module, 'field_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field[$setting] = NULL;
    }
  }
  return $field;
}

/**
 * Create an array of default values for a field instance.
 */
function content_instance_default_values($field_name, $type_name, $widget_type) {
  $widget_types = _content_widget_types();
  $module = $widget_types[$widget_type]['module'];

  $widget = array(
    'field_name' => $field_name,
    'type_name' => $type_name,
    'weight' => 0,
    'label' => $field_name,
    'description' => '',
    'widget_type' => $widget_type,
    'widget_module' => $module,
    'display_settings' => array(),
    'widget_settings' => array(),
  );

  if (module_exists($module)) {
    $widget['widget_active'] = 1;
  }

  $settings_names = array_merge(array('label'), array_keys(_content_admin_display_contexts()));
  $widget['display_settings'] = array();
  foreach ($settings_names as $name) {
    $widget['display_settings'][$name]['format'] = ($name == 'label') ? 'above' : 'default';
  }

  // Make sure widget settings all have an index in the array.
  $settings_names = module_invoke($module, 'widget_settings', 'save', $widget);
  $widget['widget_settings'] = array();
  foreach ((array) $settings_names as $name) {
    $widget['widget_settings'][$name] = NULL;
  }
  return $widget;
}

/**
 * Expand field info to create field => widget info.
 */
function content_field_instance_expand($field) {
  if (isset($field['widget'])) {
    return $field;
  }
  $field['widget'] = !empty($field['widget_settings']) ? $field['widget_settings'] : array();
  $field['widget']['label'] = !empty($field['label']) ? $field['label'] : $field['field_name'];
  $field['widget']['weight'] = !empty($field['weight']) ? $field['weight'] : NULL;
  $field['widget']['description'] = !empty($field['description']) ? $field['description'] : NULL;

  // Only fill in widget/module and widget/type if we do have valid information.
  if (!empty($field['widget_module'])) {
    $field['widget']['module'] = $field['widget_module'];
  }
  elseif (!empty($field['widget_type'])) {
    $widget_types = _content_widget_types();
    $field['widget']['module'] = $widget_types[$field['widget_type']]['module'];
  }
  if (!empty($field['widget_type'])) {
    $field['widget']['type'] = $field['widget_type'];
  }

  unset($field['widget_type']);
  unset($field['weight']);
  unset($field['label']);
  unset($field['description']);
  unset($field['widget_module']);
  unset($field['widget_settings']);

  // If content.module is handling the default value,
  // initialize $widget_settings with default values,
  if (isset($field['default_value']) && isset($field['default_value_php']) &&
      content_callback('widget', 'default value', $field) == CONTENT_CALLBACK_DEFAULT) {
    $field['widget']['default_value'] = !empty($field['default_value']) ? $field['default_value']  : NULL;
    $field['widget']['default_value_php'] = !empty($field['default_value_php']) ? $field['default_value_php'] : NULL;
    unset($field['default_value']);
    unset($field['default_value_php']);
  }
  return $field;
}

/**
 * Collapse field info from field => widget to flattened form values.
 */
function content_field_instance_collapse($field) {
  if (!isset($field['widget'])) {
    return $field;
  }
  $field['widget_settings'] = !empty($field['widget']) ? $field['widget'] : array();
  $field['widget_type'] = !empty($field['widget']['type']) ? $field['widget']['type'] : '';
  $field['weight'] = !empty($field['widget']['weight']) ? $field['widget']['weight'] : 0;
  $field['label'] = !empty($field['widget']['label']) ? $field['widget']['label'] : $field['field_name'];
  $field['description'] = !empty($field['widget']['description']) ? $field['widget']['description'] : '';
  $field['type_name'] = !empty($field['type_name']) ? $field['type_name'] : '';

  if (!empty($field['widget']['module'])) {
    $widget_module = $field['widget']['module'];
  }
  elseif (!empty($field['widget']['type'])) {
    $widget_types = _content_widget_types();
    $widget_module = $widget_types[$field['widget']['type']]['module'];
  }
  else {
    $widget_module = '';
  }
  $field['widget_module'] = $widget_module;
  unset($field['widget_settings']['type']);
  unset($field['widget_settings']['weight']);
  unset($field['widget_settings']['label']);
  unset($field['widget_settings']['description']);
  unset($field['widget_settings']['module']);
  unset($field['widget']);
  return $field;
}

/**
 * Create a new field instance.
 *
 * @param $field
 *   An array of properties to create the field with, input either in
 *   the field => widget format used by the content module or as an
 *   array of form values.
 *
 *   Required values:
 *   - field_name, the name of the field to be created
 *   - type_name, the content type of the instance to be created
 *
 *   If there is no prior instance to create this from, we also need:
 *   - type, the type of field to create
 *   - widget_type, the type of widget to use
 */
function content_field_instance_create($field) {
  include_once('./'. drupal_get_path('module', 'content') .'/content_admin.inc');

  $form_values = $field;
  $field = content_field_instance_expand($field);

  // If there are prior instances, fill out missing values from the prior values,
  // otherwise get missing values from default values.
  $prior_instances = content_field_instance_read(array('field_name' => $field['field_name']));
  if (!empty($prior_instances) && is_array($prior_instances)) {
    $prev_field = content_field_instance_expand($prior_instances[0]);

    // Weight, label, and description may have been forced into the $field
    // by content_field_instance_expand(). If there is a previous instance to
    // get these values from and there was no value supplied originally, use
    // the previous value.
    $field['widget']['weight'] = isset($form_values['weight']) ? $form_values['weight'] : $prev_field['widget']['weight'];
    $field['widget']['label']  = isset($form_values['label']) ? $form_values['label'] : $prev_field['widget']['label'];
    $field['widget']['description'] = isset($form_values['description']) ? $form_values['description'] : $prev_field['widget']['description'];
  }
  else {
    $prev_field = array('widget' => array());
  }

  // If we have a field type, we can build default values for this field type.
  $default_values = array('widget' => array());
  if (isset($field['type'])) {
    $default_values = content_field_default_values($field['type']);
    $default_instance_values = content_instance_default_values($field['field_name'], $field['type_name'], $field['widget']['type']);
    $default_values = content_field_instance_expand(array_merge($default_values, $default_instance_values));
  }

  // Merge default values, previous values, and current values to create
  // a complete field array.
  $widget = array_merge($default_values['widget'], $prev_field['widget'], $field['widget']);
  $field = array_merge($default_values, $prev_field, $field);
  $field['widget'] = $widget;

  // Make sure we know what module to invoke for field info.
  if (empty($field['module']) && !empty($field['type'])) {
    $field_types = _content_field_types();
    $field['module'] = $field_types[$field['type']]['module'];
  }

  // The storage type may need to be updated.
  $field['db_storage'] = content_storage_type($field);

  // Get a fresh copy of the column information whenever a field is created.
  $field['columns'] = (array) module_invoke($field['module'], 'field_settings', 'database columns', $field);

  if (empty($prev_field['widget']) || $prior_instances < 1) {
    // If this is the first instance, create the field.
    $field['db_storage'] = $field['multiple'] > 0 ? CONTENT_DB_STORAGE_PER_FIELD : CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
    _content_field_write($field, 'create');
  }
  elseif (!empty($prev_field['widget']) && $prev_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE && count($prior_instances) > 0) {
    // If the database storage has changed, update the field and previous instances.
    $field['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;

    foreach ($prior_instances as $instance) {
      $new_instance = $instance;
      $new_instance['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;

      // Invoke hook_content_fieldapi().
      module_invoke_all('content_fieldapi', 'update instance', $new_instance);

      content_alter_schema($instance, $new_instance);
    }
  }

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'create instance', $field);

  // Update the field and the instance with the latest values.
  _content_field_write($field, 'update');
  _content_field_instance_write($field, 'create');

  content_alter_schema(array(), $field);

  content_clear_type_cache(TRUE);
  content_menu_needs_rebuild(TRUE);

  return $field;
}

/**
 * Update an existing field instance.
 *
 * @param $field
 *   An array of properties to update the field with, input either in
 *   the field => widget format used by the content module or as an
 *   array of form values.
 */
function content_field_instance_update($field) {
  include_once('./'. drupal_get_path('module', 'content') .'/content_admin.inc');

  // Get the previous value from the table.
  $previous = content_field_instance_read(array('field_name' => $field['field_name'], 'type_name' => $field['type_name']));
  $prev_field = array_pop($previous);

  $field = content_field_instance_expand($field);

  // Create a complete field array by merging the previous and current values,
  // letting the current values overwrite the previous ones.
  $widget = array_merge($prev_field['widget'], $field['widget']);
  $field = array_merge($prev_field, $field);
  $field['widget'] = $widget;

  // Make sure we know what module to invoke for field info.
  if (empty($field['module']) && !empty($field['type'])) {
    $field_types = _content_field_types();
    $field['module'] = $field_types[$field['type']]['module'];
  }

  // The storage type may need to be updated.
  $field['db_storage'] = content_storage_type($field);

  // Changes in field values may affect columns, or column
  // information may have changed, get a fresh copy.
  $field['columns'] = (array) module_invoke($field['module'], 'field_settings', 'database columns', $field);

  // If the database storage has changed, update the field and previous instances.
  $prior_instances = content_field_instance_read(array('field_name' => $field['field_name']));

  if ($prev_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE && count($prior_instances) > 1) {

    // Update the field's data storage.
    $field['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;

    // Update the schema for prior instances to adapt to the change in db storage.
    foreach ($prior_instances as $instance) {
      if ($instance['type_name'] != $field['type_name']) {
        $new_instance = $instance;
        $new_instance['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;

        // Invoke hook_content_fieldapi().
        module_invoke_all('content_fieldapi', 'update instance', $new_instance);

        content_alter_schema($instance, $new_instance);
      }
    }
  }

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'update instance', $field);

  // Update the field and the instance with the latest values.
  _content_field_write($field, 'update');
  _content_field_instance_write($field, 'update');

  content_alter_schema($prev_field, $field);

  content_clear_type_cache(TRUE);

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
  $field['global_settings'] = array();
  $setting_names = module_invoke($field['module'], 'field_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field['global_settings'][$setting] = isset($field[$setting]) ? $field[$setting] : '';
      unset($field[$setting]);
    }
  }
  // 'columns' is a reserved word in MySQL4, so our column is named 'db_columns'.
  $field['db_columns'] = $field['columns'];

  switch ($op) {
    case 'create':
      drupal_write_record(content_field_tablename(), $field);
      break;
    case 'update':
      drupal_write_record(content_field_tablename(), $field, 'field_name');
      break;
  }
  unset($field['db_columns']);
  return $field;
}

/**
 * Write a field instance record.
 *
 * @param $field
 *   The field array to process.
 */
function _content_field_instance_write($field, $op = 'update') {
  // Collapse the field => widget format.
  $field = content_field_instance_collapse($field);

  // Rearrange the data to create the widget_settings array.
  $setting_names = module_invoke($field['widget_module'], 'widget_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field['widget_settings'][$setting] = isset($field[$setting]) ? $field[$setting] : '';
      unset($field[$setting]);
    }
  }

  switch ($op) {
    case 'create':
      drupal_write_record(content_instance_tablename(), $field);
      break;
    case 'update':
      drupal_write_record(content_instance_tablename(), $field, array('field_name', 'type_name'));
      break;
  }
  return $field;
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
    $cond = " AND ". implode(' AND ', $cond);
  }
  $db_result = db_query("SELECT * FROM {". content_instance_tablename() ."} nfi ".
    " JOIN {". content_field_tablename() ."} nf ON nfi.field_name = nf.field_name ".
    " WHERE nf.active = 1 AND nfi.widget_active = 1 ". $cond ." ORDER BY nfi.weight ASC, nfi.label ASC", $args);

  $fields = array();
  while ($instance = db_fetch_array($db_result)) {
    // 'columns' is a reserved word in MySQL4, so our column is named 'db_columns'.
    $instance['columns'] = (array) (!empty($instance['db_columns']) ? unserialize($instance['db_columns']) : array());
    unset($instance['db_columns']);
    $instance['global_settings'] = (array) (!empty($instance['global_settings']) ? unserialize($instance['global_settings']) : array());

    foreach ($instance['global_settings'] as $key => $value) {
      $instance[$key] = $value;
    }
    unset($instance['global_settings']);

    if (!empty($instance['widget_settings'])) {
      $instance['widget_settings'] = (array) unserialize($instance['widget_settings']);
    }
    else {
       $instance['widget_settings'] = array();
    }
    if (!empty($instance['display_settings'])) {
      $instance['display_settings'] = (array) unserialize($instance['display_settings']);
    }
    else {
      $instance['display_settings'] = array();
    }

    $field = content_field_instance_expand($instance);

    // Invoke hook_content_fieldapi().
    module_invoke_all('content_fieldapi', 'read instance', $field);
    $fields[] = $field;
  }
  return $fields;
}

/**
 * Delete an existing field instance.
 */
function content_field_instance_delete($field_name, $type_name) {
  include_once('./'. drupal_get_path('module', 'content') .'/content_admin.inc');

  // Get the previous field value.
  $field = array_pop(content_field_instance_read(array('field_name' => $field_name, 'type_name' => $type_name)));

  // Invoke hook_content_fieldapi().
  module_invoke_all('content_fieldapi', 'delete instance', $field);

  db_query("DELETE FROM {". content_instance_tablename() .
    "} WHERE field_name = '%s' AND type_name = '%s'", $field['field_name'], $field['type_name']);

  // If no instances remain, delete the field entirely.
  $instances = content_field_instance_read(array('field_name' => $field_name));
  if (sizeof($instances) < 1) {

    db_query("DELETE FROM {". content_field_tablename() ."} WHERE field_name = '%s'", $field['field_name']);

    // Deleted fields require a menu rebuild to remove all the field paths.
    content_menu_needs_rebuild(TRUE);
  }

  // If only one instance remains, we may need to change the database
  // representation for this field.
  if (sizeof($instances) == 1 && !($field['multiple'])) {

    // Multiple-valued fields are always stored per-field-type.
    $instance = $instances[0];
    $new_instance = $instance;
    $new_instance['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
    _content_field_write($new_instance, 'update');

    content_alter_schema($instance, $new_instance);
  }

  content_clear_type_cache(TRUE);
  return $field;
}

/**
 * Delete all data related to a module.
 *
 * @param string $module
 */
function content_module_delete($module) {
  // Delete the field data.
  $results = db_query("SELECT field_name, type_name FROM {". content_instance_tablename() ."} WHERE widget_module = '%s'", $module);
  while ($field = db_fetch_array($results)) {
    content_field_instance_delete($field);
  }
  // Force the caches and static arrays to update to the new info.
  _content_type_info(TRUE, TRUE);

}

/**
 * Make changes needed when a content type is created.
 *
 * @param $info
 *   value supplied by hook_node_type()
 *
 * node_get_types() is still missing the new type at this point, so no
 * use to call content_clear_type_cache() or menu_rebuild() here. Instead
 * we set a flag to make sure it gets rebuilt on next page load.
 */
function content_type_create($info) {
  content_menu_needs_rebuild(TRUE);
}

/**
 * Make changes needed when an existing content type is updated.
 *
 * @param $info
 *   value supplied by hook_node_type()
 */
function content_type_update($info) {
  if (!empty($info->old_type) && $info->old_type != $info->type) {
    // Rename the content type in all fields that use changed content type.
    db_query("UPDATE {". content_instance_tablename() ."} SET type_name='%s' WHERE type_name='%s'", array($info->type, $info->old_type));

    // Rename the content fields table to match new content type name.
    $old_type = content_types($info->old_type);
    $old_name = _content_tablename($old_type['type'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    $new_name = _content_tablename($info->type, CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    if (db_table_exists($old_name)) {
      $ret = array();
      db_rename_table($ret, $old_name, $new_name);
      drupal_set_message(t('Content fields table %old_name has been renamed to %new_name and field instances have been updated.', array(
        '%old_name' => $old_name, '%new_name' => $new_name)));
    }

    // Rename the variable storing weights for non-CCK fields.
    if ($extra = variable_get('content_extra_weights_'. $info->old_type, array())) {
      variable_set('content_extra_weights_'. $info->type, $extra);
      variable_del('content_extra_weights_'. $info->old_type);
    }
  }

  // Reset all content type info and alter the menu paths.
  content_menu_needs_rebuild(TRUE);
}

/**
 * Make changes needed when a content type is deleted.
 *
 * @param $info
 *   value supplied by hook_node_type()
 */
function content_type_delete($info) {
  $fields = content_field_instance_read(array('type_name' => $info->type));
  foreach ($fields as $field) {
    content_field_instance_delete($field['field_name'], $info->type);
  }
  $table = _content_tablename($info->type, CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
  if (db_table_exists($table)) {
    $ret = array();
    db_drop_table($ret, $table);
    drupal_set_message(t('The content fields table %name has been deleted.', array('%name' => $table)));
  }

  content_menu_needs_rebuild(TRUE);
}