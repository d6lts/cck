<?php
// $Id$

/**
 * @defgroup fieldapi Field API
 * @{
 *
 * The Field API allows modules to attach content fields to content
 * types.
 *
 * Terminology:
 *
 * A Field Type is a data type implemented by a module: text,
 * nodereference, address, etc. The module defines semantics and
 * functionality.
 *
 * A Field is a specific configuration of a type.  Fields have
 * settings that define the unique characteristics of that field.  All
 * fields have some core properties, such as: type, name, required,
 * cardinality (temporarily "multiple"), shareable.  A Field Type can
 * define addititional properties; for example, the Text field type
 * defines the property "formatted" which can indicate "plain text" or
 * "user chooses input format".
 *
 * A Field's settings cannot be changed because changing any of them
 * constitute fundamentally altering the field's identity.  However, a
 * field can be "converted" to a new field (of the same or different
 * field type) to provide new functionality or semantics for the
 * existing data.  TODO: Field conversion is not yet implemented.
 *
 * A Field Instance is the binding of a Field to a Content Type.
 * Field Instances have properties that define the association, such
 * as: display name (label), weight, input widget, display format.  A
 * field instance's properties do not affect the field's underlying
 * data and can be changed without altering the field itself.
 *
 * TODO: More docs.  We need to separate Field API consumer
 * documentation (e.g. above) from field module implementor
 * documentation (e.g. below).
 *
 * A hook_field() is available for each field instance action,
 * and each hook receives the nested field => widget array as an argument.
 *
 * The hook_field_$ops() include:
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
 * Retrieve the default properties for the defined element type.
 */
function _field_all_properties($type, $refresh = NULL) {
  static $cache;

  $basic_defaults = array(
    '#description' => NULL,
    '#attributes' => array(),
    '#required' => FALSE,
  );
  if (!isset($cache) || $refresh) {
    $cache = array();
    foreach (module_implements('field_type_properties') as $module) {
      $elements = module_invoke($module, 'field_type_properties');
      if (isset($elements) && is_array($elements)) {
        $cache = array_merge_recursive($cache, $elements);
      }
    }
    if (sizeof($cache)) {
      foreach ($cache as $element_type => $info) {
        $cache[$element_type] = array_merge_recursive($basic_defaults, $info);
      }
    }
  }

  return $cache[$type];
}


/**
 * Base Field data structure.
 *
 * This structure represents a Field object to the Field API.
 *
 ** Public properties
 * public $field_name;
 * public $type;
 * public $required = 0;
 * public $multiple = 0;
 * public $shareable = 0;

 ** Internal properties.  These are protected if possible.
 * public $module;
 * public $active = 0;
 * public $columns = array();
 * public $db_storage;
 */

/**
 * Construct a Field structure with default properties:
 *
 *   required: FALSE
 *   shareable: FALSE
 *   multiple: 0 (single-valued)
 *
 * NOTE: No per-field_type settings (e.g. 'format' for text fields)
 * are initialized by this function.  Use a Field Type's constructor
 * instead.
 *
 * @param $field_name
 *  The field name to create.
 * @param $field_type
 *  The field type to create; e.g. text, number, nodereference.
 */
function field_new($field_name, $field_type) {
  $field = array();
  $field['field_name'] = $field_name;
  $field['type'] = $field_type;
  $field_types = _content_field_types();
  $module = $field_types[$field['type']]['module'];
  $field['required'] = 0;
  $field['multiple'] = 0;
  $field['shareable'] = 0;
  $field['module'] = $module;
  $field['active'] = module_exists($module) ? 1 : 0;
  $field['columns'] = module_invoke($module, 'field_settings', 'database columns', $field);
  // Ensure columns always default to NULL values.
  foreach ((array) $field['columns'] as $column_name => $column) {
    $field['columns'][$column_name]['not null'] = FALSE;
  }

  $field['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;

  // Create all per-field-type properties.
  $settings = module_invoke($module, 'field_settings', 'save', $field);
  foreach ((array)$settings as $setting) {
    $field[$setting] = NULL;
  }

  return $field;
}

/**
 * Base Widget Settings data structure.  Each FieldInstance structure
 * contains one.  A field module can add additional widget settings.
 *
 * public $label;
 * public $weight;
 * public $description;
 * public $type;
 * public $module;
 */

/**
 * Base Field Instance data structure.
 *
 * This structure represents a Field Instance object to the Field API.
 *
 ** Public properties
 * public $field_name;
 * public $type_name;
 * public $widget;
 * public $default_value;
 * public $default_value_php;
 * public $display_settings = array();

 ** Internal properties.  These are protected if possible.
 * public $widget_active;
 * protected $field;
 * protected $_field_name;
 * protected $widget_class = 'WidgetSettings';
 */

/**
 * Construct a Field Instance structure with default properties.
 *
 * NOTE: No per-widget-type settings (e.g. rows) are initialized by
 * this function.  Use the Field Type's instance constructor instead.
 *
 * @param $field_name
 *  The field name to bind to a content type.
 * @param $type_name
 *  The content type to which to bind the field.
 */
function field_instance_new($field_name, $type_name, $widget_type) {
  $widget_types = _content_widget_types();
  $module = $widget_types[$widget_type]['module'];

  $instance['field_name'] = $field_name;
  $instance['type_name'] = $type_name;

  $instance['widget']['weight'] = 0;
  $instance['widget']['label'] = $field_name;
  $instance['widget']['description'] = '';
  $instance['widget']['type'] = $widget_type;
  $instance['widget']['module'] = $module;

  $instance['display_settings'] = array();

  if (module_exists($module)) {
    $instance['widget_active'] = 1;
  }

  $settings_names = array_merge(array('label'));
// TODO D7 : see about _content_admin_display_contexts ?
//  $settings_names = array_merge(array('label'), array_keys(_content_admin_display_contexts()));
  foreach ($settings_names as $name) {
    $instance['display_settings'][$name]['format'] = ($name == 'label') ? 'above' : 'default';
  }

  // Make sure widget settings all have an index in the array.
  $settings = module_invoke($module, 'widget_settings', 'save', $instance);
  foreach ((array) $settings as $setting) {
    $instance['widget'][$setting] = NULL;
  }

  return $instance;
}

/**
 * Retrieve the Field this object is an instance of.
 *
 * @param $instance
 *   A field instance.
 * @return
 *   The Field object or NULL if $instance['field_name'] is not a valid
 *   field name.
 */
function field_instance_get_field($instance) {
  if (!isset($instance['field']) || $instance['_field_name'] != $instance['field_name']) {
    $fields = field_get_fields(array('field_name' => $instance['field_name']));
    if (count($fields) != 1) {
      return NULL;
    }
    $instance['field'] = reset($fields);
    $instance['_field_name'] = $instance['field_name'];
  }
  return $instance['field'];
}

/**
 * Create a new field in the database.  This function does not assign
 * the field to any content types or create storage tables for the
 * field in the database.
 *
 * @param $field
 *   A Field structure to create.
 * @return
 *  TRUE on success, FALSE on failure.
 */
function field_create_field($field) {
  // Field name is required.
  if (empty($field['field_name'])) {
    field_error(t('Attempt to create an unnamed field.'));
    return FALSE;
  }

  // Ensure the field name is unique.
  $prior_field = field_get_fields(array('field_name' => $field['field_name']));
  if (!empty($prior_field)) {
    field_error(t('Attempt to create field name %name which already exists.', array('%name' => $field['field_name'])));
    return FALSE;
  }

  // Make sure we know what module to invoke for field info.
  if (empty($field['module']) && !empty($field['type'])) {
    $field_types = _content_field_types();
    $field['module'] = $field_types[$field['type']]['module'];
    // YCHED : we shouldn't allow incoming 'module' (or at least always overwrite it
  }
  // YCHED : error if no field type ?

  // The storage type may need to be updated.
  $field['db_storage'] = content_storage_type($field);

  // Get a fresh copy of the column information whenever a field is created.
  $field['columns'] = (array) module_invoke($field['module'], 'field_settings', 'database columns', $field);

  // Save the new field definition.
  _field_write_field($field, 'create');

  // Keep our caches up to date.
  content_clear_type_cache(TRUE);

  return TRUE;
}

/**
 * Create a new field instance.  This function assigns the field to a
 * content type and create storage tables for the field in the
 * database if necessary.
 *
 * @param $instance
 *   A Field Instance structure to create.
 * @return
 *   TRUE on success, FALSE on failure.
 */
function field_create_instance($instance) {
  include_once('./'. drupal_get_path('module', 'content') .'/includes/content.alter.inc');

  // $instance['type_name'] must exist
  if (node_get_types('type', $instance['type_name']) == FALSE) {
    field_error(t('Attempt to instantiate field for non-existant content type %type.', array('%type' => $instance['type_name'])));
    return FALSE;
  }

  // Get the Field structure this is will be an instance of.
  $field = field_instance_get_field($instance);
  if ($field == NULL) {
    field_error(t('Attempt to instantiate non-existant field %name.', array('%name' => $instance['field_name'])));
    return FALSE;
  }

  // Get prior instances of this field, if any.
  $prior_instances = field_get_instances(array('field_name' => $instance['field_name']));

  // Non-shareable fields cannot have more than one instance.
  if (!$field['shareable'] && count($prior_instances) > 0) {
    field_error(t('Attempt to re-instantiate non-shareable field %name.', array('%name' => $field['field_name'])));
    return FALSE;
  }

  // Invoke hook_field().
  module_invoke_all('field_create_instance', $instance);

  // Create the new instance record.
  _field_write_instance($instance, 'create');

  // Create the field columns/tables for the new instance, if necessary.
  if (!$field['shareable'] || count($prior_instances) == 0) {
    content_alter_schema(NULL, $instance);
  }

  // Keep our caches up to date.
  content_clear_type_cache(TRUE);
  menu_rebuild();

  return $instance;
}

/**
 * Update an existing field instance.  This function can only change
 * Field Instance properties, not Field properties.
 *
 * @param $new_instance
 *   A Field Instance structure.  The instance matching the field_name
 *   and type_name in this structure will be updated to match the
 *   other properties set in the structure.
 * @return
 *   $instance on success, FALSE on failure.
 */
function field_update_instance($new_instance) {
  // Verify that the instance already exists.
  $previous = field_get_instances(array('field_name' => $new_instance['field_name'], 'type_name' => $new_instance['type_name']));
  if (count($previous) != 1) {
    field_error(t('Attempt to update a non-existant field instance %name/%type.', array('%name' => $new_instance['field_name'], '%type' => $new_instance['type_name'])));
    return FALSE;
  }

  // Invoke hook_field().
  module_invoke_all('field_update_instance', $new_instance);

  // Update the instance record with the latest values.
  _field_write_instance($new_instance, 'update');

  // Keep our caches up to date.
  content_clear_type_cache(TRUE);
  menu_rebuild();

  return $new_instance;
}

/**
 * Delete an existing field instance.  Someone is going to say this
 * should take a FieldInstance structure for uniformity, and they will
 * probably be right.  TODO: Untested!
 *
 * @param $field_name
 *   The name of the field instance to delete.
 * @param $type_name
 *   The content type of the field instance to delete.
 */
function field_delete_instance($field_name, $type_name) {
  include_once('./'. drupal_get_path('module', 'content') .'/includes/content.alter.inc');

  // Get the previous field value.
  $instance = field_get_instance($field_name, $type_name);

  // TODO : error if no instance.
  // Invoke hook_field().
  module_invoke_all('field_delete_instance', $instance);

  // Delete the instance record from our table.
  db_delete(content_instance_tablename())
    ->condition('field_name', $instance['field_name'])
    ->condition('type_name', $instance['type_name'])
    ->execute();
//  db_query("DELETE FROM {". content_instance_tablename() .
//    "} WHERE field_name = :field_name AND type_name = :type_name", array(':field_name' => $instance['field_name'], ':type_name' => $instance['type_name']));

  // If no instances remain, delete the database storage entirely.
  // This is always the case for non-shareable fields and may be the
  // case for shareable fields.
  $remaining = field_get_instances(array('field_name' => $field_name), TRUE);
  if (count($remaining) == 0) {
    content_alter_schema($instance, NULL);
  }

  // Keep our caches up to date.
  content_clear_type_cache(TRUE);
  menu_rebuild();

  return $instance;
}

/**
 * Write a field record.
 *
 * @param $field
 *   The field array to process.
 */
// TODO D7 : could the $op be ditched using a merge query ?
function _field_write_field($field, $op = 'update') {
  // Rearrange the data to create the global_settings array.
  $field['global_settings'] = array();
  $setting_names = module_invoke($field['module'], 'field_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field['global_settings'][$setting] = isset($field[$setting]) ? $field[$setting] : '';
      unset($field[$setting]); // XXX why do this?
    }
  }
  // 'columns' is a reserved word in MySQL4, so our column is named 'db_columns'.
  $field['db_columns'] = $field['columns'];
  unset($field['columns']);

  switch ($op) {
    case 'create':
      drupal_write_record(content_field_tablename(), $field);
      break;
    case 'update':
      drupal_write_record(content_field_tablename(), $field, 'field_name');
      break;
  }
  $field['columns'] = $field['db_columns'];
  unset($field['db_columns']);
  // TODO D7 : do we want the return value ?
  return $field;
}

function _content_field_instance_array2db($instance) {
  if (!isset($instance['widget'])) {
    return $instance;
  }
  $instance['widget_settings'] = !empty($instance['widget']) ? $instance['widget'] : array();
  $instance['widget_type'] = !empty($instance['widget']['type']) ? $instance['widget']['type'] : '';
  $instance['weight'] = !empty($instance['widget']['weight']) ? $instance['widget']['weight'] : 0;
  $instance['label'] = !empty($instance['widget']['label']) ? $instance['widget']['label'] : $instance['field_name'];
  $instance['description'] = !empty($instance['widget']['description']) ? $instance['widget']['description'] : '';
  $instance['type_name'] = !empty($instance['type_name']) ? $instance['type_name'] : '';

  if (!empty($instance['widget']['module'])) {
    $widget_module = $instance['widget']['module'];
  }
  elseif (!empty($instance['widget']['type'])) {
    $widget_types = _content_widget_types();
    $widget_module = $widget_types[$instance['widget']['type']]['module'];
  }
  else {
    $widget_module = '';
  }
  $instance['widget_module'] = $widget_module;
  unset($instance['widget_settings']['type']);
  unset($instance['widget_settings']['weight']);
  unset($instance['widget_settings']['label']);
  unset($instance['widget_settings']['description']);
  unset($instance['widget_settings']['module']);
  unset($instance['widget']);
  return $instance;
}


/**
 * Write a field instance record.
 *
 * @param $instance
 *   The instance structure to process.
 */
// TODO D7 : could the $op be ditched using a merge query ?
function _field_write_instance($instance, $op = 'update') {
  $instance = _content_field_instance_array2db($instance);
  // Rearrange the data to create the widget_settings array.
  // YCHED: Why isn't _content_field_instance_array2db() enough ?
//  $setting_names = module_invoke($instance['widget']['module'], 'widget_settings', 'save', $instance);
//  if (is_array($setting_names)) {
//    foreach ($setting_names as $setting) {
//      $instance['widget'][$setting] = isset($instance['widget'][$setting])? $instance['widget'][$setting] : '';
//    }
//  }
//
//  // TODO: fix this
//  $instance['widget_settings'] = $instance['widget'];

  switch ($op) {
    case 'create':
      drupal_write_record(content_instance_tablename(), $instance);
      break;
    case 'update':
      drupal_write_record(content_instance_tablename(), $instance, array('field_name', 'type_name'));
      break;
  }
  return $instance;
}

/**
 * Load a field.
 *
 * @param $param
 *   An array of properties to use in selecting fields. Valid keys:
 *   - 'field_name' - The name of the field.
 *   - 'type' - The type of the field.
 *   If NULL, all fields will be returned.
 * @param $include_inactive
 *   TRUE will return fields that are 'inactive', because their field
 *   module is currently disabled.
 * @return
 *   The fields array.
 */
function field_get_fields($param, $include_inactive = FALSE) {
  $query = db_select(content_field_tablename(), 'f', array('fetch' => PDO::FETCH_ASSOC))
    ->fields('f');

  if (is_array($param)) {
    // Turn the conditions into a query.
    foreach ($param as $key => $value) {
      if (in_array($key, array('field_name', 'type'))) {
        $query->condition($key, $value);
      }
    }
  }
  if (!$include_inactive) {
    $query->condition('active', 1);
  }
  $results = $query->execute();

  $fields = array();
  foreach ($results as $row) {
    $function = $row['type'] . '_field_new';
    $field = $function($row['field_name']);

    // 'columns' is a reserved word in MySQL4, so our column is named 'db_columns'.
    $field['columns'] = (array) (!empty($row['db_columns']) ? unserialize($row['db_columns']) : array());
    unset($row['db_columns']);

    $global_settings = (array) (!empty($row['global_settings']) ? unserialize($row['global_settings']) : array());
    foreach ($global_settings as $key => $value) {
      $field[$key] = $value;
    }

    foreach ($row as $key => $value) {
      $field[$key] = $value;
    }

    // Invoke hook_field().
    module_invoke_all('field_read_field', $field);
    $fields[$field['field_name']] = $field;
  }
  return $fields;
}

/**
 * Load a field instance.
 *
 * @param $field_name
 *   A field name.
 * @param $type_name
 *  A content type name.
 * @return
 *  The specified field instance, or NULL if it does not exist.
 */
function field_get_instance($field_name, $type_name) {
  $instances = field_get_instances(array('field_name' => $field_name, 'type_name' => $type_name));
  if (!empty($instances)) {
    // There will never be more than one.
    return $instances[0];
  }
}

/**
 * Loads field instances matching certain criteria.
 *
 * @param $param
 *   An array of properties to use in selecting a field instance. Valid keys:
 *   - 'type_name' - The name of the content type in which the instance exists.
 *   - 'field_name' - The name of the field whose instance is to be loaded.
 *   if NULL, all instances will be returned.
 * @param $include_inactive
 *   TRUE will return field instances that are 'inactive', because their field
 *   module or widget module is currently disabled.
 * @return
 *   The field arrays.
 */
function field_get_instances($param = NULL, $include_inactive = FALSE) {
  $query = db_select(content_instance_tablename(), 'fi', array('fetch' => PDO::FETCH_ASSOC));
  $query->join(content_field_tablename(), 'f', 'fi.field_name = f.field_name');
  $query->fields('fi');
  $query->addField('f', 'type');
  $query->orderBy('weight');
  $query->orderBy('label');

  if (is_array($param)) {
    // Turn the conditions into a query.
    foreach ($param as $key => $value) {
      if (in_array($key, array('field_name', 'type_name'))) {
        $query->condition("fi.$key", $value);
      }
    }
  }
  if (!$include_inactive) {
    $query->condition('f.active', 1);
    $query->condition('fi.widget_active', 1);
  }
  $results = $query->execute();

  $instances = array();
  foreach ($results as $row) {
    // YCHED : defer massaging to _content_field_instance_db2array() ?
    // YCHED : de we need to call '_field_instance_new' ?
    $function = $row['type'] . '_field_instance_new';
    $widget = unserialize($row['widget_settings']);
    $instance = $function($row['field_name'], $row['type_name'], $row['widget_type']);

    $instance['widget'] = $widget;
    $instance['widget']['type'] = $row['widget_type'];
    $instance['widget']['module'] = $row['widget_module'];
    // YCHED: bad !
    $instance['widget']['label'] = $row['label'];
    $instance['widget']['description'] = $row['description'];
    $instance['widget']['weight'] = $row['weight'];
    if (!empty($row['display_settings'])) {
      $instance['display_settings'] = (array) unserialize($row['display_settings']);
    }

    // XXX TODO: default_value properties are not handled correctly
    // If content.module is handling the default value,
    // initialize widget_settings with default values,
    if (isset($instance['default_value']) && isset($instance['default_value_php']) &&
      content_instance_callback('default value', $instance) == CONTENT_CALLBACK_DEFAULT) {
      $instance['widget']['default_value'] = !empty($instance['default_value']) ? $instance['default_value']  : NULL;
      $instance['widget']['default_value_php'] = !empty($instance['default_value_php']) ? $instance['default_value_php'] : NULL;
      unset($instance['default_value']);
      unset($instance['default_value_php']);
    }

    // Invoke hook_field().
    module_invoke_all('field_read_instance', $instance);
    $instances[] = $instance;
  }
  return $instances;
}

/**
 * Delete all data related to a module.
 *
 * @param string $module
 */
function content_module_delete($module) {
  // TODO D7 : study this case...
  // Delete the field data.
  $results = db_query("SELECT field_name, type_name FROM {". content_instance_tablename() ."} WHERE widget_module = :module", array(':module' => $module), array('fetch' => PDO::FETCH_ASSOC));
  foreach($results as $field) {
    // content_field_instance_delete($field['field_name'], $field['type_name']);
  }
  // Force the caches and static arrays to update to the new info.
  content_clear_type_cache(TRUE);
}

/**
 * Make changes needed when a content type is created.
 *
 * @param $info
 *   value supplied by hook_node_type()
 *
 * node_get_types() is still missing the new type at this point due to
 * a static caching bug. We ask it to rebuild its cache so that
 * content_clear_type_cache() can do its job properly.
 */
function content_type_create($info) {
  node_get_types(NULL, NULL, TRUE);
  content_clear_type_cache(TRUE);
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
    db_update(content_instance_tablename())
      ->fields(array('type_name' => $info->type))
      ->condition('type_name', $info->old_type)
      ->execute();

    // Rename the content fields table to match new content type name.
    $old_type = content_types($info->old_type);
    $old_name = _content_tablename($old_type['type'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    $new_name = _content_tablename($info->type, CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    if (db_table_exists($old_name)) {
      $ret = array();
      db_rename_table($ret, $old_name, $new_name);
      watchdog('content', 'Content fields table %old_name has been renamed to %new_name and field instances have been updated.', array(
        '%old_name' => $old_name, '%new_name' => $new_name));
    }
  }

  // Reset all content type info.
  // Menu needs to be rebuilt as well, but node types need to be rebuilt first.
  // node_type_form_submit() takes care of this.
  content_clear_type_cache(TRUE);
}

/**
 * Make changes needed when a content type is deleted.
 *
 * @param $info
 *   value supplied by hook_node_type()
 *
 * TODO should we skip doing this entirely since core leaves the
 * nodes in the database as orphans and wait until the nodes are
 * deleted to respond?
 */
function content_type_delete($info) {
  // Don't delete data for content-types defined by disabled modules.
  if (!empty($info->disabled)) {
    return;
  }

  // TODO : What about inactive fields ?
  // Currently, content_field_instance_delete doesn't work on those...
  $fields = field_get_instances(array('type_name' => $info->type));
  foreach ($fields as $field) {
    content_field_instance_delete($field['field_name'], $info->type);
  }
  $table = _content_tablename($info->type, CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
  if (db_table_exists($table)) {
    $ret = array();
    db_drop_table($ret, $table);
    watchdog('content', 'The content fields table %name has been deleted.', array('%name' => $table));
  }

  // Menu needs to be rebuilt as well, but node types need to be rebuilt first.
  // node_type_form_submit() takes care of this.
  content_clear_type_cache(TRUE);
}

//class FieldException extends Exception {}
function field_error($msg) {
  watchdog('content', $msg);
  if (module_exists('devel')) {
    dsm($msg);
  }
  //throw new FieldException($msg);
}

/**
 * @} End of "defgroup fieldapi".
 */
