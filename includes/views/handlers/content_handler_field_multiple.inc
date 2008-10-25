<?php
// $Id$

/**
 * @file
 * An extended subclass for field handling that adds multiple field grouping.
 *
 * Fields that want multiple value grouping options in addition to basic
 * field and formatter handling can extend this class.
 */
class content_handler_field_multiple extends content_handler_field {
  var $defer_query;

  function init(&$view, $options) {
    $field = $this->content_field;
    parent::init($view, $options);
    if ($field['multiple']) {
      $this->additional_fields['delta'] = 'delta';
    }
    $this->additional_fields['type'] = array('table' => 'node', 'field' => 'type');
  }

  function options(&$options) {
    parent::options($options);
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
      '#weight' => 1,
    );
    $form['multiple']['group'] = array(
      '#title' => t('Group multiple values'),
      '#type' => 'checkbox',
      '#default_value' => $options['multiple']['group'],
      '#description' => t('If unchecked, each item in the field will create a new row, which may appear to cause duplicates. This setting is not compatible with click-sorting in table displays.'),
    );
    // Make the string translatable by keeping it as a whole rather than
    // translating prefix and suffix separately.
    list($prefix, $suffix) = explode('@count', t('Show @count value(s)'));
    $form['multiple']['multiple_number'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => $prefix,
      '#field_suffix' => $suffix,
      '#default_value' => $options['multiple']['multiple_number'],
      '#prefix' => '<div class="container-inline">',
      '#process' => array('views_process_dependency'),
      '#dependency' => array('edit-options-multiple-group' => array(TRUE)),
    );
    list($prefix, $suffix) = explode('@count', t('starting from @count'));
    $form['multiple']['multiple_from'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => $prefix,
      '#field_suffix' => $suffix,
      '#default_value' => $options['multiple']['multiple_from'],
      '#process' => array('views_process_dependency'),
      '#dependency' => array('edit-options-multiple-group' => array(TRUE)),
    );
    $form['multiple']['multiple_reversed'] = array(
      '#title' => t('Reversed (start from last values)'),
      '#type' => 'checkbox',
      '#default_value' => $options['multiple']['multiple_reversed'],
      '#suffix' => '</div>',
      '#process' => array('views_process_dependency'),
      '#dependency' => array('edit-options-multiple-group' => array(TRUE)),
    );
  }

  /**
   * Determine if this field is click sortable.
   */
  function click_sortable() {
    $field = $this->content_field;
    $options = $this->options;

    return !empty($this->definition['click sortable']) && !($options['multiple']['group'] && $field['multiple']);
  }

  function query() {
    $field = $this->content_field;
    $options = $this->options;

    // 'Group multiple values' : the values for the field will be retrieved
    // in a separate query (one per field).
    $this->defer_query = $options['multiple']['group'] && $field['multiple'];

    // Non-grouped field: use the generic field handler query.
    if (!$this->defer_query) {
      return parent::query();
    }

    // Grouped field: overwrite the existing additional_fields, because they
    // would cause duplicate rows for multiple values. The only field we add
    // is vid, which will be used to retrieve the actual values in pre_render.
    $this->additional_fields = array('vid' => array('table' => 'node', 'field' => 'vid'));
    $this->add_additional_fields();
    $this->field_alias = $this->aliases['vid'];

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
    // Build the list of vids to retrieve.
    // TODO: try fetching from cache_content first ??
    $vids = array();
    $this->field_values = array();
    foreach ($values as $result) {
      if (isset($result->{$this->field_alias})) {
        $vids[] = $result->{$this->field_alias};
      }
    }

    // List columns to retrieve.
    $alias = content_views_tablename($field);

    $query_columns = array(
      'node.type',
      "$alias.vid AS parent_vid",
      "$alias.delta"
    );
    foreach ($db_info['columns'] as $column => $attributes) {
      $query_columns[] = "$alias.$attributes[column] AS $column";
    }
    $query = 'SELECT '. implode(', ', $query_columns) .
             ' FROM {'. $db_info['table'] ."} $alias".
             " LEFT JOIN {node} node ON node.vid = $alias.vid".
             " WHERE $alias.vid IN (". implode(',', $vids) .')'.
             " ORDER BY node.nid ASC, $alias.delta ". ($options['multiple']['multiple_reversed'] ? 'DESC' : 'ASC');
    $result = db_query($query);

    while ($item = db_fetch_array($result)) {
      $delta = !empty($item['delta']) ? $item['delta'] : 0;
      $item['#delta'] = $item['delta'];
      unset($item['delta']);
      $this->field_values[$item['parent_vid']][$delta] = $item;
    }
  }

  function render($values) {
    // If this is not a grouped field, use content_handler_field::render().
    if (!$this->defer_query || empty($this->field_values)) {
      return parent::render($values);
    }

    $field = $this->content_field;
    $options = $this->options;

    // $values will be used as a fake $node object;
    // we provide a build_mode for rendering.
    $values->build_mode = NODE_BUILD_NORMAL;

    // This needs to be set for the $this->render_link() to work. It would
    // have been set in the query, if we hadn't bypassed the normal query.
    // TODO there may be a better way to do this.
    $this->aliases['nid'] = 'nid';

    $vid = $values->{$this->field_alias};
    if (isset($this->field_values[$vid])) {
      // Gather items, respecting the 'Display n values starting from m' settings.
      $count_skipped = 0;
      $items = array();
      foreach ($this->field_values[$vid] as $item) {
        if (empty($options['multiple']['multiple_from']) || ($count_skipped >= $options['multiple']['multiple_from'])) {
          if (empty($options['multiple']['multiple_number']) || (count($items) < $options['multiple']['multiple_number'])) {
            $items[] = $item;
          }
          else {
            break;
          }
        }
        $count_skipped++;
      }

      // Render items.
      $formatter_name = $options['format'];
      if ($items && ($formatter = _content_get_formatter($formatter_name, $field['type']))) {
        $rendered = array();
        if (content_handle('formatter', 'multiple values', $formatter) == CONTENT_HANDLE_CORE) {
          // Single-value formatter.
          foreach ($items as $item) {
            $output = content_format($field, $item, $formatter_name, $values);
            if (!empty($output)) {
              $rendered[] = $this->render_link($output, $values);
            }
          }
        }
        else {
          // Multiple values formatter.
          $output = content_format($field, $items, $formatter_name, $values);
          if (!empty($output)) {
            $rendered[] = $this->render_link($output, $values);
          }
        }

        if (count($rendered) > 1) {
          // TODO: could we use generic field display ?
          return theme('content_view_multiple_field', $rendered, $field, $values);
        }
        elseif ($rendered) {
          return $rendered[0];
        }
      }
    }

    return '';
  }
}