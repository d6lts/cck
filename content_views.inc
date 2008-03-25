<?php
// $Id$

/**
 * @file
 * Interface between content.module and views.module.
 */

views_include_handlers();

/**
 * Helper function so it is possible to change the Views tablename
 * in the future without re-writing code.
 */
function content_views_tablename($field) {
  return $table_alias = 'node_data_'. $field['field_name'];
}

/**
 * Implementation of hook_views_data().
 *
 * Exposes all fields to the views system.
 */
function content_views_data() {
  $data = array();
  foreach (content_fields() as $field) {
    $module = $field['module'];
    $result = module_invoke($module, 'field_settings', 'views data', $field);
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

  // Check the field module is available.
  // TODO : is this really how we should do it ?
  if (isset($field_types[$field['type']])) {
    $db_info = content_database_info($field);
    $table_alias = content_views_tablename($field);

    $types = array();
    foreach(content_types() as $type) {
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

    // Identify likely filters for each column based on field type.
    $filters = array();
    foreach($db_info['columns'] as $column => $attributes) {
      switch ($attributes['type']) {
        case 'numeric':
        case 'int':
        case 'mediumint':
        case 'tinyint':
        case 'bigint':
        case 'serial':
        case 'float':
          $filters[] = 'views_handler_filter_numeric_content';
          break;
        default:
          $filters[] = 'views_handler_filter_string_content';
          break;
      }
    }

    $data[$columns[0]] = array(
      'group' => t('Field'),
      'title' => t($field['widget']['label']) .' ('. $field['field_name'] .')',
      'help' =>  t($field_types[$field['type']]['label']) .' - '. t('Appears in : @types', array('@types' => implode(', ', $types))),
      'field' => array(
        'field' => $columns[0],
        'tablename' => $db_info['table'],
        'handler' => 'views_handler_field_content',
        'click sortable' => '', // TODO
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      ),
      'argument' => array(
        'field' => $columns[0],
        'tablename' => $db_info['table'],
        'handler' => 'views_handler_argument_content',
        'name field' => '', // TODO
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      ),
      'filter' => array(
        'field' => $columns[0],
        'title' => t($field['widget']['label']),
        'tablename' => $db_info['table'],
        'handler' => $filters[0],
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      ),
      'sort' => array(
        'field' => $columns[0],
        'tablename' => $db_info['table'],
        'handler' => 'views_handler_sort',
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      ),
    );

    // Add an empty/not empty filter for each field.
    // TODO explore ways to incorporate this into other filters
    // so each field will have only one filter that includes this option.
    $data[$columns[0] .'_empty'] = array(
      'group' => t('Field'),
      'title' => t('@field empty', array('@field' => $field['widget']['label'])) .' ('. $field['field_name'] .')',
      'help' =>  t($field_types[$field['type']]['label']) .' - '. t('Appears in : @types', array('@types' => implode(', ', $types))),
      'filter' => array(
        'field' => $columns[0],
        'title' => t('@field empty', array('@field' => $field['widget']['label'])),
        'tablename' => $db_info['table'],
        'handler' => 'views_handler_filter_content_is_empty',
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      ),
    );

    // TODO : provide several filters, arguments, sorts... ?
    // TODO : provide automatic filters for each column, not just the first?
    return array($table_alias => $data);
  }
}

class views_handler_field_content extends views_handler_field_node {
  var $content_field;
  var $defer_query;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
  }

  /**
   * Called to determine what to tell the clicksorter.
   */
//  function click_sort() {
//    return "$this->field_alias";
//  }

  function options(&$options) {
    parent::options($options);
    $field = $this->content_field;

    // Override views_handler_field_node's default label
    $options['label'] = $field['widget']['label'];

    $options['format'] = 'default';
    $options['multiple'] = array(
      'group' => TRUE,
      'multiple_number' => '',
      'multiple_from' => '',
      'multiple_reversed' => FALSE,
    );
  }

  /**
   * Provide formatter and 'group multiple values' option.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    // TODO : do we want the 'link to node' checkbox ? That's usually formatters business...

    $field = $this->content_field;
    $options = $this->options;

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
      '#default_value' => $options['format'],
    );

    $form['multiple'] = array(
      '#access' => $field['multiple'],
    );
    $form['multiple']['group'] = array(
      '#title' => t('Group Multiple values'),
      '#type' => 'checkbox',
      '#default_value' => $options['multiple']['group'],
    );
    $form['multiple']['multiple_number'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => t('Show '),
      '#field_suffix' => t(' values,'),
      '#default_value' => $options['multiple']['multiple_number'],
      '#prefix' => '<div class="container-inline">',
    );
    $form['multiple']['multiple_from'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => t('starting from '),
      '#default_value' => $options['multiple']['multiple_from'],
    );
    $form['multiple']['multiple_reversed'] = array(
      '#title' => t('Start from last values'),
      '#type' => 'checkbox',
      '#default_value' => $options['multiple']['multiple_reversed'],
      '#suffix' => '</div>',
    );
  }

  function options_validate($form, &$form_state) { }

  /**
   * Provide text for the administrative summary
   */
  function admin_summary() {
    // TODO : how come "Field : field_texte" is displayed (and not "Text : field_texte") ?
    // Display the formatter name.
    $field = $this->content_field;
    $field_types = _content_field_types();
    if (isset($field_types[$field['type']]['formatters'][$this->options['format']])) {
      return t($field_types[$field['type']]['formatters'][$this->options['format']]['label']);
    }
  }

  function query() {
    $field = $this->content_field;
    $options = $this->options;

//  if (in_array($field['handler'], array('content_views_field_handler_group', 'content_views_field_handler_first', 'content_views_field_handler_last'))) {
// TODO : multiple_number / multiple_from
// We can also optimize the query when only one value should be retrieved
    $this->defer_query = $options['multiple']['group'] && $field['multiple'];

    if ($this->defer_query) {
      // We don't add anything to the query. The data is retrieved in pre_render()
      // in order to avoid duplicate results.
      $this->field_alias = $this->real_field;

      // If the field is sortable (table sort), we just have to join the table
      // ("Node: Distinct" will be required to avoid duplicates...)
//      if ($field['sortable']) {
//        $this->ensure_my_table();
//      }
    }
    else {
      // Use the generic field handler query.
      parent::query();
    }
  }

  function pre_render($values) {
    $field = $this->content_field;
    $db_info = content_database_info($field);
    $options = $this->options;

    $this->field_values = array();

    if ($this->defer_query && $values) {
      // Build the list of nids to retrieve.
      // TODO : try fetching from cache_content first ??
      $nids = array();
      foreach ($values as $value) {
        $nids[] = $value->nid;
      }

      // List columns to retrieve.
      $table_alias = content_views_tablename($field);
      $query_columns = array("$table_alias.delta AS delta");
      foreach ($db_info['columns'] as $column => $attributes) {
        $query_columns[] = "$table_alias.$attributes[column] AS $column";
      }

      // Note : this query doesn't need to run through db_rewrite_sql, since the
      // nids we retrieve have been selected by the views query, which already takes
      // care of this.
      $query = "SELECT ". implode(', ', $query_columns) .
               " FROM {node} node".
               " LEFT JOIN {". $db_info['table'] ."} $table_alias ON node.vid = $table_alias.vid".
               " WHERE node.nid IN (". implode(',', $nids) .")".
               " ORDER BY nid ASC, $table_alias.delta ". ($options['multiple']['multiple_reversed'] ? 'DESC' : 'ASC');
      // TODO : Select all deltas or only a subset.
      $result = $options['multiple']['multiple_number'] ? db_query_range($query, $options['multiple']['multiple_from'], $options['multiple']['multiple_number']) : db_query($query);

      while ($item = db_fetch_array($result)) {
        $this->field_values[$item['nid']][$item['delta']] = $item;
      }
    }
    else {
      foreach ($values as $value) {
        $item = array();
        foreach ($db_info['columns'] as $column => $attributes) {
          $item[$column] = $value->{$this->table_alias .'_'. $attributes['column']};
        }
        $this->field_values[$value->{$this->additional_fields['nid']}][0] = $item;
      }
    }
  }

  function render($values) {
    $field = $this->content_field;
    $options = $this->options;

    // $values will be used as a fake $node object; we provide a build_mode
    // for rendering.
    // TODO : we can stick any value in there - what would make most sense ?  row_style ?
    $values->build_mode = 'views';

    $items = array();
    foreach ($this->field_values[$values->{$this->additional_fields['nid']}] as $item) {
      $items[] = content_format($field, $item, $options['format'], $values);
    }

    if (count($items) > 1) {
      // TODO : could we use generic field display ?
      return theme('content_view_multiple_field', $items, $field, $values);
    }
    else {
      return $items[0];
    }
  }
}

function theme_content_view_multiple_field($items, $field, $values) {
  $output = '';
  foreach ($items as $item) {
    $output .= '<div class="field-item">'. $item .'</div>';
  }
  return $output;
}

/**
 * Subclassed string filter adds content fields
 */
class views_handler_filter_string_content extends views_handler_filter_string {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
}

/**
 * Subclassed numeric filter adds content fields
 */
class views_handler_filter_numeric_content extends views_handler_filter_numeric {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
}

/**
 * Subclassed 'in' filter adds content fields
 */
class views_handler_filter_in_operator_content extends views_handler_filter_in_operator {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
}

/**
 * Subclassed empty/not empty filter adds content fields and empty/not empty handler
 */
class views_handler_filter_content_is_empty extends views_handler_filter_boolean_operator {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }


  function admin_summary() {
    if (!empty($this->options['exposed'])) {
      return t('exposed');
    }

    return ($this->value == 'NULL') ? t('is True') : t('is False');
  }

  function value_form(&$form, &$form_state) {
    $form['value'] = array(
      '#type' => 'select',
      '#options' => array('NOT NULL' => t('Not empty'), 'NULL' => t('Empty')),
      '#default_value' => $this->value,
      '#field_prefix' => 'Is ',
    );
  }

  function query() {
    $this->ensure_my_table();
    if ($this->value == 'NULL') {
      foreach ($this->additional_fields as $column) {
        $this->query->add_where($this->options['group'], "$this->table_alias.$column IS NULL");
      }
    }
    else {
      // TODO : does groups allow something smarter ?
      $where = array();
      foreach ($this->additional_fields as $column) {
        $where[] = "$this->table_alias.$column IS NOT NULL";
      }
      $this->query->add_where($this->options['group'], implode(") OR (", $where));
    }
  }
}

/**
 * The views_handler_argument_content subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class views_handler_argument_content extends views_handler_argument {
  var $content_field;

  function construct() {
    parent::construct();

    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
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
          $filter_name = ' - '.$filter_name;
        }
        $name = $field_types[$field['type']]['label'] .': '. $field['widget']['label'] . $filter_name .' ('. $field['field_name'] .')';
        $init = array(
          'name' => $name,
//          'field' => $main_column['column'],
//          'content_db_info' => $db_info,
//          'content_field' => $field,
//          'content_field_module' => $module,
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
        case 'numeric':
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

