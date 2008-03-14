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
    foreach (content_fields() as $field) {
      $module = $field['module'];
      $result = module_invoke($module, 'field_settings', 'views data', $field);
      if (empty($result)) {
        // In Views 1, the views field name had the column name appended,
        // like field_name_value.
        $views_fields[$field['field_name'] .'_'. $field['columns'][0]] = $field;
      }
    }
  }

  if (!in_array($field['field'], array_keys($views_fields))) {
    return;
  }
  // For Views 2, switch back to using the field name.
  $views_field['field'] = $views_fields[$views_fields['field']['field_name']];
  $view->add_item($display, $type, $field['tablename'], $field['field']);

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

    case 'argument':
      // TODO
      return;

    case 'sort':
      // TODO
      break;

  }
  return;
}