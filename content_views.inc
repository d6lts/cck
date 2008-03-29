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
    $data['table']['group']  = $field_types[$field['type']]['label'];
    $data['table']['join']['node'] = array(
      'handler' => 'views_join',
      'arguments' => array($db_info['table'], 'node', 'vid', 'vid')
    );

    $columns = array();
    $arguments = array();
    $filters = array();
    foreach($db_info['columns'] as $column => $attributes) {
      $columns[] = $attributes['column'];
      $sorts[] = !empty($attributes['sortable']) ? TRUE : FALSE;

      // Identify likely filters and arguments for each column based on field type.
      switch ($attributes['type']) {
        case 'numeric':
        case 'int':
        case 'mediumint':
        case 'tinyint':
        case 'bigint':
        case 'serial':
        case 'float':
          $filters[] = 'views_handler_filter_numeric_content';
          $arguments[] = 'views_handler_argument_content';
          break;
        default:
          $filters[] = 'views_handler_filter_string_content';
          $arguments[] = 'views_handler_argument_string_content';
          break;
      }
    }

    $data[$columns[0]] = array(
      'group' => t($field_types[$field['type']]['label']),
      'title' => t($field['widget']['label']) .' ('. $field['field_name'] .')',
      'help' =>  t($field_types[$field['type']]['label']) .' - '. t('Appears in : @types', array('@types' => implode(', ', $types))),
      'field' => array(
        'field' => $columns[0],
        'tablename' => $db_info['table'],
        'handler' => 'views_handler_field_content_multiple',
        'click sortable' => '', // TODO
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      ),
      'argument' => array(
        'field' => $columns[0],
        'tablename' => $db_info['table'],
        'handler' => $arguments[0],
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
    );
    // TODO do we need different handling for sorts with Views 2,
    // especially when relationships are involved?
    if (!empty($sorts[0])) {
      $data[$columns[0]]['sort'] = array(
        'field' => $columns[0],
        'tablename' => $db_info['table'],
        'handler' => 'views_handler_sort_content',
        'additional fields' => $columns,
        'content_field_name' => $field['field_name'],
      );
    }

    // TODO : provide automatic filters, sorts, and arguments for each column, not just the first?
    return array($table_alias => $data);
  }
}

/**
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class views_handler_filter_string_content extends views_handler_filter_string {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
    $this->definition['allow empty'] = TRUE;
  }
}

/**
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class views_handler_filter_numeric_content extends views_handler_filter_numeric {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
    $this->definition['allow empty'] = TRUE;
  }
}

/**
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class views_handler_filter_in_operator_content extends views_handler_filter_in_operator {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
    $field = $this->content_field;
    $this->value_title = $field['widget']['label'];
    $this->value_options = $this->allowed_values();
  }

  // Get allowed values from hook_allowed_values(), if any,
  // or from content_allowed_values();
  function allowed_values() {
    $field = $this->content_field;
    $function = $field['module'] .'_allowed_values';
    $options = function_exists($function) ? $function($field) : content_allowed_values($field);
    return (array) $options;
  }

  function options(&$options) {
    parent::options($options);
    // Set the default value to the first value, so that there is something
    // reasonable there.
    $keys = array_keys($this->allowed_values());
    $options['value'] = array(array_shift($keys));
  }
}

/**
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class views_handler_sort_content extends views_handler_sort {
  var $content_field;

  function construct() {
    parent::construct();

    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
}

/**
 * The subclass simply adds properties,
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
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class views_handler_argument_string_content extends views_handler_argument_string {
  var $content_field;

  function construct() {
    parent::construct();

    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }
}

/**
 * The subclass adds basic field and formatter info,
 * for field-specific subclasses to use if they need to.
 *
 * Fields could extend this class if they want field and formatter handling
 * but don't want the multiple value grouping options created by
 * views_handler_field_content_multiple.
 */
class views_handler_field_content extends views_handler_field_node {
  var $content_field;

  function construct() {
    parent::construct();
    $this->content_field = content_fields($this->definition['content_field_name']);
  }

  /**
   * This provides a method of controlling field-level access in Views.
   */
  function access() {
    // Avoid FALSE returns for non-existant hooks by using module_implements()
    // to check only modules that implement the hook.
    $modules = module_implements('content_views_access');
    foreach ($modules as $module) {
      if (module_invoke($module, 'content_views_access', $this->definition['content_field_name']) === FALSE) {
        return FALSE;
      }
    }
    return parent::access();
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
  }

  /**
   * Provide formatter option.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    // TODO : do we want the 'link to node' checkbox ?
    // That's usually formatters business...

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
  }

  function options_validate($form, &$form_state) { }

  /**
   * Provide text for the administrative summary
   */
  function admin_summary() {
    // Display the formatter name.
    $field = $this->content_field;
    $field_types = _content_field_types();
    if (isset($field_types[$field['type']]['formatters'][$this->options['format']])) {
      return t($field_types[$field['type']]['formatters'][$this->options['format']]['label']);
    }
  }

  function render($values) {
    $field = $this->content_field;
    $options = $this->options;
    $db_info = content_database_info($field);

    // $values will be used as a fake $node object;
    // we provide a build_mode for rendering.
    // TODO : we can stick any value in there -
    // what would make most sense ?  row_style ?
    $values->build_mode = 'views';
    $item = array();
    foreach ($db_info['columns'] as $column => $attributes) {
      $item[$column] = $values->{$this->table_alias .'_'. $attributes['column']};
    }
    return content_format($field, $item, $options['format'], $values);
  }
}

/**
 * An extended subclass for field handling that adds multiple field grouping.
 *
 * Fields that want multiple value grouping options in addition to basic
 * field and formatter handling can extend this class.
 */
class views_handler_field_content_multiple extends views_handler_field_content {
  var $defer_query;

  function options(&$options) {
    parent::options($options);
    $field = $this->content_field;
    $options['multiple'] = array(
      'group' => TRUE,
      'multiple_number' => '',
      'multiple_from' => '',
      'multiple_reversed' => FALSE,
    );
  }

  /**
   * Provide 'group multiple values' option.
   */
  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $field = $this->content_field;
    $options = $this->options;

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

  function query() {
    $field = $this->content_field;
    $options = $this->options;

    //  if (in_array($field['handler'], array('content_views_field_handler_group', 'content_views_field_handler_first', 'content_views_field_handler_last'))) {
    // TODO : multiple_number / multiple_from
    // We can also optimize the query when only one value should be retrieved
    $this->defer_query = $options['multiple']['group'] && $field['multiple'];

    // If this is not a grouped field, use the generic field handler query.
    if (!$this->defer_query) {
      return parent::query();
    }

    // We don't add anything to the query. The data is retrieved in pre_render()
    // in order to avoid duplicate results.
    $this->field_alias = $this->real_field;

    // If the field is sortable (table sort), we just have to join the table
    // ("Node: Distinct" will be required to avoid duplicates...)
    // if ($field['sortable']) {
    //   $this->ensure_my_table();
    // }
  }

  function pre_render($values) {
    // There are no values to render in a summary view.
    if (isset($this->view->build_info['summary'])) {
      return parent::pre_render($values);
    }
    // If this is not a grouped field, use the parent pre_render().
    if (!$this->defer_query || empty($values)) {
      return parent::pre_render($values);
    }

    $field = $this->content_field;
    $db_info = content_database_info($field);
    $options = $this->options;

    $this->field_values = array();
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
    $query = "SELECT node.nid, ". implode(', ', $query_columns) .
             " FROM {node} node".
             " LEFT JOIN {". $db_info['table'] ."} $table_alias ON node.vid = $table_alias.vid".
             " WHERE node.nid IN (". implode(',', $nids) .")".
             " ORDER BY node.nid ASC, $table_alias.delta ". ($options['multiple']['multiple_reversed'] ? 'DESC' : 'ASC');
    // TODO : Select all deltas or only a subset.
    $result = $options['multiple']['multiple_number'] ? db_query_range($query, $options['multiple']['multiple_from'], $options['multiple']['multiple_number']) : db_query($query);

    while ($item = db_fetch_array($result)) {
      $this->field_values[$item['nid']][$item['delta']] = $item;
    }
  }

  function render($values) {
    // If this is not a grouped field, use the parent render().
    if (!$this->defer_query) {
      return parent::render($values);
    }

    $field = $this->content_field;
    $options = $this->options;

    // $values will be used as a fake $node object;
    // we provide a build_mode for rendering.
    // TODO : we can stick any value in there -
    // what would make most sense ?  row_style ?
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
