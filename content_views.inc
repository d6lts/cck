<?php
// $Id$

/**
 * @file
 * Interface between content.module and views.module.
 */

/**
 * Implementation of hook_views_tables().
 *
 * Exposes all fields to the views system if the field has delegated its database
 * storage to content.module.
 */
function content_views_tables() {
  $field_types = _content_field_types();

  $tables = array();
  foreach (content_fields() as $field) {
    $db_info = content_database_info($field);
    $module = $field_types[$field['type']]['module'];
    
    $formatters = array();
    if (is_array($field_types[$field['type']]['formatters'])) {
      foreach ($field_types[$field['type']]['formatters'] as $name => $info) {
        $formatters[$name] = t($info['label']);
      }
    }

    if (count($db_info['columns'])) {
      $table = array();

      $table['name'] = $db_info['table'];
      $table['join'] = array(
        'left' => array(
          'table' => 'node',
          'field' => 'vid',
        ),
        'right' => array(
          'field' => 'vid',
        ),
      );

      $columns = $db_info['columns'];
      $main_column = array_shift($columns);
      $addlfields = array();
      foreach ($columns as $column => $attributes) {
        $addlfields[] = $attributes['column'];
      }

      $table['fields'] = array();
      $table['fields'][$main_column['column']] = array(
        'name' => $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'] .')',
        'addlfields' => $addlfields,
        'sortable' => isset($main_column['sortable']) ? $main_column['sortable'] : FALSE,
        'handler' => 'content_views_field_handler',
        'option' => array('#type' => 'select', '#options' => $formatters),
        'content_db_info' => $db_info,
        'content_field' => $field,
        'content_field_module' => $module,
      );
      if (isset($main_column['sortable']) && $main_column['sortable']) {
        $table['sorts'] = array();
        $table['sorts'][$main_column['column']] = array(
          'name' => $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'] .')',
          'field' => $main_column['column'],
        );
      }

      $filters = module_invoke($module, 'field_settings', 'filters', $field);
      if (is_array($filters) && count($filters)) {
        $table['filters'] = array();
        foreach ($filters as $key => $filter) {
          $filter_name = '';
          if (count($filters) > 1) {
            $filter_name = (!empty($filter['name'])) ? $filter['name'] : $key;
            $filter_name = ' - '.$filter_name;
          }
          $name = $field_types[$field['type']]['label'] .': '. $field['widget']['label'] . $filter_name .' ('. $field['field_name'] .')';
          $init = array(
            'name' => $name,
            'field' => $main_column['column'],
          );
          $table['filters'][$main_column['column'] .'_'. $key] = array_merge($filter, $init);
        }
      }

      // We don't use $db_info['table'] for the key, since that may change during
      // the lifetime of the field and we don't want to require users to redefine
      // their views.
      $tables['node_data_'. $field['field_name']] = $table;
    }
  }
  return $tables;
}

function content_views_field_handler($field_info, $field_data, $value, $data) {
  $field = $field_info['content_field'];
  $item = array();
  foreach ($field_info['content_db_info']['columns'] as $column => $attributes) {
    $view_column_name = 'node_data_'. $field['field_name'] .'_'. $attributes['column'];
    $item[$column] = $data->$view_column_name;
  }
  return content_format($field, $item, $field_data['options']);
}

/**
 * Implementation of hook_views_arguments().
 *
 * Exposes all fields as filterable arguments if the field has delegated its database
 * storage to content.module.
 */
function content_views_arguments() {
  $field_types = _content_field_types();
  $arguments = array();

  foreach (content_fields() as $field) {
    $db_info = content_database_info($field);
    $module = $field_types[$field['type']]['module'];
    if (count($db_info['columns'])) {
      $main_column = reset($db_info['columns']);
      $default_arguments = isset($main_column['default arguments']) ? $main_column['default arguments'] : TRUE;
      $argument = array();
      $argument['name'] = $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'] .')';
      $argument['handler'] = 'content_views_argument_handler';

      $arguments['content: '. $field['field_name']] = $argument;
    }
  }
  return $arguments;
}

/**
 * Perform filtering by an argument for field data stored via content.module.
 */
function content_views_argument_handler($op, &$query, $argtype, $arg = '') {
  if ($op == 'filter') {
    $field_name = substr($argtype['type'], 9);
  }
  else {
    $field_name = substr($argtype, 9);
  }

  $field = content_fields($field_name);
  $db_info = content_database_info($field);
  $main_column = reset($db_info['columns']);

  // The table name used here is the Views alias for the table, not the actual
  // table name.
  $table = 'node_data_'. $field['field_name'];

  switch ($op) {
    case 'summary':
      $query->ensure_table($table);
      $query->add_field($main_column['column'], $table);
      return array('field' => $table .'.'. $main_column['column']);
      break;

    case 'sort':
      break;

    case 'filter':
      $query->ensure_table($table);
      switch ($main_column['type']) {
        case 'int':
        case 'mediumint':
        case 'tinyint':
        case 'bigint':
          $column_placeholder = '%d';
          break;
        case 'float':
          $column_placeholder = '%f';
          break;
        default:
          $column_placeholder = "'%s'";
      }
      $query->add_where($table .'.'. $main_column['column'] .' = '. $column_placeholder, $arg);
      break;

    case 'link':
      $item = array();
      foreach ($db_info['columns'] as $column => $attributes) {
        $view_column_name = $attributes['column'];
        $item[$column] = $query->$view_column_name;
      }

      return l(content_format($field, $item, 'plain'), $arg .'/'. $query->$main_column['column'], array(), NULL, NULL, FALSE, TRUE);

    case 'title':
      $item = array(key($db_info['columns']) => $query);

      return content_format($field, $item);
  }
}
