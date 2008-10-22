<?php
// $Id$

/**
 * @file
 * The subclass simply adds properties,
 * for field-specific subclasses to use if they need to.
 */
class content_handler_sort extends views_handler_sort {
  var $content_field;

  function construct() {
    parent::construct();

    $this->content_field = content_fields($this->definition['content_field_name']);
    $this->additional_fields = $this->definition['additional fields'];
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['delta'] = array('default' => 0);

    return $options;
  }

  /**
   * Add a delta selector for multiple fields.
   */
  function options_form(&$form, &$form_state) {
    $field = $this->content_field;
    parent::options_form($form, $form_state);

    // Only add the form gadget if the field is multiple.
    if ($field['multiple']) {
      $max_delta = $field['multiple'];
      // 1 means unlimited.
      if ($max_delta == 1) {
        $max_delta = 10;
      }

      $options = array();
      for ($i = 0; $i < $max_delta; $i++) {
        $options[$i] = $i + 1;
      }
      $form['delta'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => $this->options['delta'],
        '#title' => t('Delta'),
        '#description' => t('The delta allows you to select which item in a multiple value field will be used for sorting. Select "1" to use the first item, "2" for the second item, and so on..'),
      );
    }
  }

  function ensure_my_table() {
    if (!isset($this->table_alias)) {
      $join = $this->get_join();
      if (!isset($join->extra)) {
        $join->extra = array();
      }
      $delta = isset($this->options['delta']) ? $this->options['delta'] : 0;
      $join->extra[] = array(
        'field' => 'delta',
        'value' => $delta,
        'numeric' => TRUE,
      );

      $this->table_alias = $this->query->add_table($this->table, $this->relationship, $join);
    }
    return $this->table_alias;
  }

  function query() {
    $this->ensure_my_table();
    // Add the field.
    $this->query->add_orderby($this->table_alias, $this->real_field, $this->options['order']);
  }
}