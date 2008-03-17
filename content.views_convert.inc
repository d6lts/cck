<?php
//$Id$
/**
 * @file
 * Field conversion for fields handled by this module.
 */
/**
 * Implementation of hook_views_convert().
 *
 * Intervene to convert field values from the Views 1 format to the
 * Views 2 format. Intervene only if $view->add_item() won't produce
 * the right results, usually needed to set field options or values.
 */
function content_views_convert($display, $type, &$view, $views_field) {
  static $views_fields;

  if (empty($views_fields)) {
    $views_fields = array();
    $types = content_types();
    foreach ($types as $ctype) {
      foreach ($ctype['fields'] as $field) {
        $module = $field['module'];
        $result = module_invoke($module, 'field_settings', 'views data', $field);
        if (empty($result)) {
          // In Views 1, the views field name had the column name appended,
          // like field_name_value or field_username_uid.
          $column = array_shift(array_keys($field['columns']));
          $views_fields[$field['field_name'] .'_'. $column] = $field;
        }
      }
    }
  }
  // Is this a field that CCK should handle? If not, return.
  if (!in_array($views_field['field'], array_keys($views_fields))) {
    return;
  }
  // For Views 2, switch back to using the field name, make sure a
  // handler exists, and add the field. The changed field name means
  // that the first attempt to add the field would have failed.
  $field = $views_field;
  $field['field'] = $views_fields[$views_field['field']]['field_name'];
  if ($type != 'exposed_filter' && views_get_handler($field['tablename'], $field['field'], $type)) {
    $view->add_item($display, $type, $field['tablename'], $field['field']);
  }
  else {
    return;
  }

  // Now update values, options, etc. to those selected in the imported view.
  switch ($type) {
    case 'field':
      $view->display[$type]->display_options['fields'][$field['field']]['format'] = $field['options'];
      if ($field['handler'] == 'content_views_field_handler_group') {
        $view->display[$type]->display_options['fields'][$field['field']]['multiple']['group'] = 1;
      }
      else {
        $view->display[$type]->display_options['fields'][$field['field']]['multiple']['group'] = 0;
      }
      return;

    case 'filter':
      // TODO
      return;

    case 'exposed_filter':
      // Because of the change in field name, the exposed filters never got set up correctly.
      // Not enough information provided to tell which exposed filter this is,
      // need to submit a Views patch to pass that info. This will work as long as there
      // is only one exposed filter.
      $count = 1;

      $view->display[$display]->display_options['filters'][$field['field']]['exposed'] = 1;
      $view->display[$display]->display_options['filters'][$field['field']]['expose']['identifier'] = 'filter' . $count;
      $view->display[$display]->display_options['filters'][$field['field']]['expose']['label'] = $field['label'];
      $view->display[$display]->display_options['filters'][$field['field']]['expose']['operator'] = $field['operator'] ? 'op' . $count : '';
      $view->display[$display]->display_options['filters'][$field['field']]['expose']['optional'] = $field['optional'];
      $view->display[$display]->display_options['filters'][$field['field']]['expose']['single'] = $field['single'];
      return;

    case 'argument':
      // TODO
      return;

    case 'sort':
      // TODO
      break;

  }
  return;
}