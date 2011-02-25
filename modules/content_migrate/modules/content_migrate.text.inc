<?php
/**
 * @file content_migrate.text.inc
 * Code to implement hook_content_migrate_field_alter, content_migrate_instance_alter() and content_migrate_data_record_alter()
 * on behalf of the former text module, moved into a separate file for efficiency.
 */

/**
 * Implements hook_content_migrate_field_alter().
 * 
 * Use this to tweak the conversion of field settings
 * from the D6 style to the D7 style for specific
 * situations not handled by basic conversion,
 * as when field types or settings are changed.
 */
function content_migrate_text_field_alter(&$field_value, $instance_value) {
  switch ($field_value['type']) {
    case 'text':

      // Text fields are translatable.      
      $field_value['translatable'] = TRUE;

      // The max_length field can no longer be empty 
      // or it will create a SQL error.
      if (empty($field_value['settings']['max_length'])) {
        $field_value['settings']['max_length'] = 255;
      }

      // The allowed values list should now be stored as an array.
      $allowed_values = array();
      if (!empty($field_value['settings']['allowed_values'])) {
        $allowed_values;
        foreach (explode("\n", $field_value['settings']['allowed_values']) as $value) {
          if (empty($value)) continue;
          if (strstr($value, '|')) {
            $parts = explode('|', $value);
            $allowed_values[trim($parts[0])] = trim($parts[1]);
          }
          else {
            $allowed_values[] = trim($value);
          }
        }
      }
      
      // Text fields using optionwidgets are
      // now List fields.
      switch ($instance_value['widget']['type']) {
        case 'optionwidgets_buttons':
        case 'optionwidgets_select':
          $field_value['type'] = 'list_text';
          $field_value['module'] = 'list';
          $field_value['settings']['allowed_values'] = $allowed_values;
          break;
        case 'optionwidgets_onoff':
          $field_value['type'] = 'list_boolean';
          $field_value['module'] = 'list';
          $field_value['settings']['allowed_values'] = $allowed_values;
          break;  
        case 'text_textarea':
          $field_value['type'] = 'text_long';
          unset($field_value['settings']['max_length']);
          break;  
      }
      break;
  }

}

/**
 * Implements hook_content_migrate_instance_alter().
 *
 * Use this to tweak the conversion of instance or widget settings
 * from the D6 style to the D7 style for specific
 * situations not handled by basic conversion, as when
 * formatter or widget names or settings are changed.
 */
function content_migrate_text_instance_alter(&$instance_value, $field_value) {
  switch ($field_value['module']) {
    case 'text':
      // The formatter names changed, all are prefixed
      // with 'text_'.
      foreach ($instance_value['display'] as $context => $settings) {
        $instance_value['display'][$context]['type'] = 'text_'. $settings['type'];
      }
      break;
  }

}
