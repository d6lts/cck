<?php
// $Id$

/**
 * @file
 * Interface between content.module and views.module.
 */

views_include_handlers();

/**
 * Implementation of hook_views_data().
 *
 * Exposes all fields to the views system.
 */
function content_views_data() {
  $data = array();
  foreach (content_fields() as $field) {
    $module = $field['module'];
    $result = module_invoke($module, 'field_settings', 'views_data', $field);
    if (empty($result)) {
      $result = content_views_field_views_data($field);
    }
    if (is_array($result)) {
      $data = array_merge($data, $result);
    }
  }
  return $data;
}


function content_views_field_views_data($field) {
  $field_types = _content_field_types();
  $db_info = content_database_info($field);
  $table_alias = 'node_data_'. $field['field_name'];

  $types = array();
  foreach (content_types() as $type) {
    if (isset($type['fields'][$field['field_name']])) {
      $types[] = $type['name'];
    }
  }

  $data = array();
  $data['table']['group']  = t('Field');
  $data['table']['join']['node'] = array(
    'handler' => 'views_join',
    'arguments' => array($db_info['table'], 'node', 'vid', 'vid')
  );

  $columns = array();
  foreach ($db_info['columns'] as $attributes) {
    $columns[] = $attributes['column'];
  }

  $data[$field['field_name']] = array(
    // TODO : we probably need the actual table and column ?
    // Group by field type.
    'group' => $field_types[$field['type']]['label'],
    'title' => $field['widget']['label'],
    'help' => $field['field_name'] .' - '. t('Appears in : @types', array('@types' => implode(', ', $types))),
    'field' => array(
      'field' => $columns[0],
      'tablename' => $db_info['table'],
      'handler' => 'views_handler_field_content',
      'arguments' => array(TRUE, $columns, $field['field_name']), // TODO : TRUE is 'sortable'
    ),
    'argument' => array(
      'field' => $columns[0],
      'tablename' => $db_info['table'],
      'handler' => 'views_handler_argument_content',
      'arguments' => array($columns, $field['field_name']), // TODO : ??
    ),
    'filter' => array(
      'field' => $columns[0],
      'tablename' => $db_info['table'],
      'handler' => 'views_handler_filter',
      'arguments' => array($columns, $field['field_name']), // TODO : ??
    ),
    'sort' => array(
      'field' => $columns[0],
      'tablename' => $db_info['table'],
      'handler' => 'views_handler_sort',
      'arguments' => array($columns, $field['field_name']), // TODO : ??
    ),
  );

  // TODO : provide several filters, arguments, sorts... ?
  // TODO : allow field modules to provide filters etc on all their columns if needed ?

  return array($table_alias => $data);
}

class views_handler_field_content extends views_handler_field_node {
  var $content_field;

  // TODO : ?
  function init(&$view, &$data) {
    parent::init($view, $data);
  }

  /**
   * Construct function.
   * Arguments are taken from the 'arguments' entry in the field definition.
   *
   * - make $field available in $this->content_field
   */
  function construct($click_sortable = FALSE, $additional_fields = array(), $field_name) {
    parent::construct($click_sortable, $additional_fields);

    $this->content_field = content_fields($field_name);
  }

  /**
   * Called to determine what to tell the clicksorter.
   */
//  function click_sort() {
//    return "$this->field_alias";
//  }

  // TODO : ?
//  function options(&$option) {
//    // add additional stuff to the field ?
//  }

  /**
   * Provide formatter and 'group multiple values' option.
   */
  function options_form(&$form) {
    // TODO : not sure we want this (link to node ... ) ?
    parent::options_form($view, $data);
    $field = $this->content_field;

    $field_types = _content_field_types();
    $formatters = array();
    if (is_array($field_types[$field['type']]['formatters'])) {
      foreach ($field_types[$field['type']]['formatters'] as $name => $info) {
        $formatters[$name] = t($info['label']);
      }
    }

    $form['format'] = array(
      '#title' => t('Format'),
      '#type' => 'select',
      '#options' => $formatters,
      '#required' => TRUE,
      '#default_value' => isset($this->data->format) ? $this->data->format : 'default',
    );

    $form['group_multiple'] = array(
      '#title' => t('Group Multiple values'),
      '#type' => 'checkbox',
      '#default_value' => isset($this->data->group_multiple) ? $this->data->group_multiple : TRUE,
    );

    $form['multiple'] = array(
      '#prefix' => '<div class="container-inline">',
      '#suffix' => '</div>',
    );
    $form['multiple']['multiple_number'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => t('Show '),
      '#field_suffix' => t(' values,'),
      '#default_value' => isset($this->data->multiple['multiple_number']) ? $this->data->multiple['multiple_number'] : '',
    );
    $form['multiple']['multiple_from'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => t('starting from '),
      '#default_value' => isset($this->data->multiple['multiple_from']) ? $this->data->multiple['multiple_from'] : '',
    );
    $form['multiple']['multiple_reversed'] = array(
      '#title' => t('Start from last values'), // TODO
      '#type' => 'checkbox',
      '#default_value' => isset($this->data->multiple['multiple_reversed']) ? $this->data->multiple['multiple_reversed'] : 0,
    );

  }

  /**
   * Validate the options form.
   */
  function options_validate($form, &$form_state) { }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit($form, &$form_state) { }

  /**
   * Provide text for the administrative summary
   */
  function admin_summary() {
    // Display the formatter name.
    $field = $this->content_field;
    $field_types = _content_field_types();
    if (isset($field_types[$field['type']]['formatters'][$this->data->format])) {
      return t($field_types[$field['type']]['formatters'][$this->data->format]['label']);
    }
  }

  /**
   * Called to add the field to a query.
   */
  function query() {
    $field = $this->content_field;
    $options = $this->data->options;

//  if (in_array($field['handler'], array('content_views_field_handler_group', 'content_views_field_handler_first', 'content_views_field_handler_last'))) {
// TODO : multiple_number / multiple_from
    if ($options['group_multiple'] && $field['multiple']) {
      // 'Group multiple values' : we don't add anything to the query.
      // The data is retrieved in render() in order to avoid duplicate results.

      // If the field is sortable (table sort), we just have to join the table
      // ("Node: Distinct" will be required to avoid duplicates...)
      if ($field['sortable']) {
        $this->ensure_my_table();
      }
    }
    else {
      // Non-multiple field or 'Group multiple values' unchecked :
      // we defer to the regular field query.
      parent::query();
    }
  }

  function render($values) {
    $field = $this->content_field;
    $options = $this->data->options;

    if ($options['group_multiple'] && $field['multiple']) {
      // 'Group multiple values' : we retrieve all the values in a separate query...
      // TODO : does Views2 allow something smarter ?

      foreach ($field['columns'] as $column => $attributes) {
        // TODO : check the alias generated by Views
        $query_columns[] = "node_data_$field[field_name].$attributes[column] AS $column";
      }
      // Add node.nid as the first item in the field list to keep db_rewrite_sql()
      // from trying to force DISTINCT on the nid field of nodereference fields.
      $query = "SELECT node.nid, ". implode(', ', $query_columns) .
               " FROM {node} node".
               // TODO : check the alias generated by Views
               " LEFT JOIN {". $field['table'] ."} node_data_$field[field_name] ON node.vid = node_data_$field[field_name].vid".
               // TODO : check $values->{$this->nid_field}
               " WHERE node.nid = ". $values->{$this->nid_field} .
               " ORDER BY node_data_$field[field_name].delta ". ($options['multiple_reversed'] ? 'ASC' : 'DESC');
      $query = db_rewrite_sql($query, 'node');
      // Select all deltas or only a subset.
      // Currently only 'all' and first / last items are proposed to the user.
      $result = $options['multiple_number'] ? db_query($query) : db_query_range($query, $options['multiple_from'], $options['multiple_number']);

      $items = array();
      while ($item = db_fetch_array($result)) {
        $items[] = content_format($field, $item, $options['format'], $values);
      }
      // TODO : could we use generic field theming here ?
      return theme('content_view_multiple_field', $items, $field, $values);
    }
    else {
      // Non-multiple field or 'Group multiple values' unchecked :
      $item = array();
      foreach ($field['columns'] as $column => $attributes) {
        // TODO : check generated alias...
        $item[$column] = $values->{$this->table_alias .'_'. $attributes['column']};
      }
      return content_format($field, $item, $field_data['options'], $values);
    }
  }
}

function theme_content_view_multiple_field($items, $field, $values) {
  foreach ($items as $item) {
    $output .= '<div class="field-item">'. $item .'</div>';
  }
  return $output;
}

class views_handler_argument_content extends views_handler_argument {
  var $content_field;

  /**
   * Init function :
   * - make $field available in $this->content_field
   */
  function init(&$view, &$data) {
    parent::init($view, $data);
    $this->content_field = content_fields($data['content_field_name']);
  }

  // TODO ...
}

/**
 * Implementation of hook_views_tables().
 *
 * Exposes all fields to the views system.
 */
//function content_views_tables() {
//  $field_types = _content_field_types();
//  $return = array();
//  foreach (content_fields() as $field) {
//    $module = $field_types[$field['type']]['module'];
//    $result = module_invoke($module, 'field_settings', 'tables', $field);
//    if (empty($result)) {
//      $result = content_views_field_tables($field);
//    }
//    if (is_array($result)) {
//      $return = array_merge($return, $result);
//    }
//  }
//  return $return;
//}

function content_views_field_tables($field) {
  $field_types = _content_field_types();
  $db_info = content_database_info($field);

  if (count($db_info['columns'])) {
//    $table = array();
//
//    $table['name'] = $db_info['table'];
//    $table['join'] = array(
//      'left' => array(
//        'table' => 'node',
//        'field' => 'vid',
//      ),
//      'right' => array(
//        'field' => 'vid',
//      ),
//    );

//    $module = $field_types[$field['type']]['module'];

//    $formatters = array();
//    if (is_array($field_types[$field['type']]['formatters'])) {
//      foreach ($field_types[$field['type']]['formatters'] as $name => $info) {
//        $formatters[$name] = t($info['label']);
//      }
//    }

//    $columns = $db_info['columns'];
//    $main_column = array_shift($columns);
//    $addlfields = array();
//    foreach ($columns as $column => $attributes) {
//      $addlfields[] = $attributes['column'];
//    }

//    $table['fields'] = array();
    $table['fields'][$main_column['column']] = array(
//      'name' => $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'] .')',
//      'addlfields' => $addlfields,
      'sortable' => isset($main_column['sortable']) ? $main_column['sortable'] : FALSE,
//      'query_handler' => 'content_views_field_query_handler',
//      'handler' => array(
//        'content_views_field_handler_group' => t('Group multiple values'),
//        'content_views_field_handler_ungroup' => t('Do not group multiple values'),
//        'content_views_field_handler_first' => t('Show first value only'),
//        'content_views_field_handler_last' => t('Show last value only'),
//      ),
//      'option' => array('#type' => 'select', '#options' => $formatters),
//      'content_db_info' => $db_info,
//      'content_field' => $field,
//      'content_field_module' => $module,
    );
    if (isset($main_column['sortable']) && $main_column['sortable']) {
      $table['sorts'] = array();
      $table['sorts'][$main_column['column']] = array(
//        'name' => $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'] .')',
        'field' => $main_column['column'],
        'content_db_info' => $db_info,
        'content_field' => $field,
        'content_field_module' => $module,
      );
    }

    $filters = module_invoke($module, 'field_settings', 'filters', $field);
    if (is_array($filters) && count($filters)) {
      $table['filters'] = array();
      foreach ($filters as $key => $filter) {
        $filter_name = '';
        if (count($filters) > 1) {
          $filter_name = (!empty($filter['name'])) ? $filter['name'] : $key;
          $filter_name = ' - '. $filter_name;
        }
        $name = $field_types[$field['type']]['label'] .': '. $field['widget']['label'] . $filter_name .' ('. $field['field_name'] .')';
        $init = array(
          'name' => $name,
          'field' => $main_column['column'],
          'content_db_info' => $db_info,
          'content_field' => $field,
          'content_field_module' => $module,
        );
        $table['filters'][$main_column['column'] .'_'. $key] = array_merge($filter, $init);
      }
    }

    // We don't use $db_info['table'] for the key, since that may change during
    // the lifetime of the field and we don't want to require users to redefine
    // their views.
    return array('node_data_'. $field['field_name'] => $table);
  }
}

//function content_views_field_query_handler($field, &$fieldinfo, &$query) {
//  if (in_array($field['handler'], array('content_views_field_handler_group', 'content_views_field_handler_first', 'content_views_field_handler_last'))) {
//    // we manage the addition of fields ourselves
//    // if not multiple field, add the columns to the query
//    if (!$fieldinfo['content_field']['multiple']) {
//      $query->add_field($field['field'], $field['tablename'], $field['queryname']);
//      foreach ($fieldinfo['addlfields'] as $name) {
//        $query->add_field($name, $field['tablename'], "$field[tablename]_$name");
//      }
//    }
//    // if multiple field, no field gets added (they are retrieved later
//    // in content_views_field_handler in order to avoid duplicate results)
//    else {
//      // if the field is sortable (table sort), then we have to join the table
//      // ("Node: Distinct" will be required to avoid duplicates...)
//      if ($field['sortable']) {
//        $query->ensure_table($field['tablename']);
//      }
//    }
//    // make sure views default query builder does not add anything
//    $fieldinfo['notafield'] = true;
//    unset($fieldinfo['addlfields']);
//  }
//}

//function content_views_field_handler_group($field_info, $field_data, $value, $data, $from = 0, $count = 'all', $order = 'ASC') {
//  $field = $field_info['content_field'];
//  $items = array();
//  if ($field['multiple']) {
//    foreach ($field_info['content_db_info']['columns'] as $column => $attributes) {
//      $query_columns[] = "node_data_$field[field_name].$attributes[column] AS $column";
//    }
//    // Add node.nid as the first item in the field list to keep db_rewrite_sql()
//    // from trying to force DISTINCT on the nid field of nodereference fields.
//    $query = "SELECT node.nid, ". implode(', ', $query_columns) .
//             " FROM {node} node".
//             " LEFT JOIN {". $field_info['content_db_info']['table'] ."} node_data_$field[field_name] ON node.vid = node_data_$field[field_name].vid".
//             " WHERE node.nid = ". $data->nid .
//             " ORDER BY node_data_$field[field_name].delta $order";
//    $query = db_rewrite_sql($query, 'node');
//    // Select all deltas or only a subset.
//    // Currently only 'all' and first / last items are proposed to the user.
//    $result = ($count == 'all') ? db_query($query) : db_query_range($query, $from, $count);
//
//    while ($item = db_fetch_array($result)) {
//      $items[] = content_format($field, $item, $field_data['options'], $data);
//    }
//    return theme('content_view_multiple_field', $items, $field, $data);
//  }
//  else {
//    return content_views_field_handler_ungroup($field_info, $field_data, $value, $data);
//  }
//}

//function content_views_field_handler_first($field_info, $field_data, $value, $data) {
//  return content_views_field_handler_group($field_info, $field_data, $value, $data, 0, 1, 'ASC');
//}
//
//function content_views_field_handler_last($field_info, $field_data, $value, $data) {
//  return content_views_field_handler_group($field_info, $field_data, $value, $data, 0, 1, 'DESC');
//}

//function content_views_field_handler_ungroup($field_info, $field_data, $value, $data) {
//  $field = $field_info['content_field'];
//  $item = array();
//  foreach ($field_info['content_db_info']['columns'] as $column => $attributes) {
//    $view_column_name = $field_data['tablename'] .'_'. $attributes['column'];
//    $item[$column] = $data->$view_column_name;
//  }
//  return content_format($field, $item, $field_data['options'], $data);
//}

/**
 * Implementation of hook_views_arguments().
 *
 * Exposes all fields as filterable arguments.
 */
//function content_views_arguments() {
//  $field_types = _content_field_types();
//  $return = array();
//  foreach (content_fields() as $field) {
//    $module = $field_types[$field['type']]['module'];
//    $result = module_invoke($module, 'field_settings', 'arguments', $field);
//    if (empty($result)) {
//      $result = content_views_field_arguments($field);
//    }
//    if (is_array($result)) {
//      $return = array_merge($return, $result);
//    }
//  }
//  return $return;
//}

//function content_views_field_arguments($field) {
//  $field_types = _content_field_types();
//  $db_info = content_database_info($field);
//  if (count($db_info['columns'])) {
//    $argument = array();
//    $argument['name'] = $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'] .')';
//    $argument['handler'] = 'content_views_argument_handler';
//
//    return array('content: '. $field['field_name'] => $argument);
//  }
//}

/**
 * Perform filtering by an argument for field data stored via content.module.
 */
function content_views_argument_handler($op, &$query, $argtype, $arg = '') {
//  if ($op == 'filter') {
//    $field_name = substr($argtype['type'], 9);
//  }
//  else {
//    $field_name = substr($argtype, 9);
//  }

//  $field = content_fields($field_name);
//  $db_info = content_database_info($field);
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

      return l(content_format($field, $item, 'plain'), $arg .'/'. $query->$main_column['column'], array('html' => TRUE));

    case 'title':
      $item = array(key($db_info['columns']) => $query);

      return content_format($field, $item, 'plain');
  }
}

