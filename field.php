<?php
// $Id$

/**
 * @file
 * These hooks are defined by field modules, modules that define a new kind
 * of field for insertion in a content type.
 *
 * Field hooks are typically called by content.module using _content_field_invoke().
 *
 * Widget module hooks are also defined here; the two go hand-in-hand, often in
 * the same module (though they are independent).
 *
 * Widget hooks are typically called by content.module using _content_widget_invoke().
 */

/**
 * @addtogroup hooks
 * @{
 */


/**
 * Declare information about a field type.
 *
 * @return
 *   An array keyed by field type name. Each element of the array is an associative
 *   array with these keys and values:
 *   - "label": The human-readable label for the field type.
 */
function hook_field_info() {
  return array(
    'number_integer' => array('label' => 'Integer'),
    'number_decimal' => array('label' => 'Decimal'),
  );
}

/**
 * Handle the parameters for a field.
 *
 * @param $op
 *   The operation to be performed. Possible values:
 *   - "form": Display the field settings form.
 *   - "validate": Check the field settings form for errors.
 *   - "save": Declare which fields to save back to the database.
 *   - "database columns": Declare the columns that content.module should create
 *     and manage on behalf of the field. If the field module wishes to handle
 *     its own database storage, this should be omitted.
 *   - "filters": If content.module is managing the database storage,
 *     this operator determines what filters are available to views.
 *     They always apply to the first column listed in the "database columns"
 *     array.
 * @param $field
 *   The field on which the operation is to be performed.
 * @return
 *   This varies depending on the operation.
 *   - The "form" operation should return an array of form elements to add to
 *     the settings page.
 *   - The "validate" operation has no return value. Use form_set_error().
 *   - The "save" operation should return an array of names of form elements to
 *     be saved in the database.
 *   - The "database columns" operation should return an array keyed by column
 *     name, with arrays of column information as values. This column information
 *     must include "type", the MySQL data type of the column, and may also
 *     include a "sortable" parameter to indicate to views.module that the
 *     column contains ordered information. Details of other information that can
 *     be passed to the database layer can be found at content_db_add_column().
 *   - The "filters" operation should return an array whose values are 'filters' 
 *     definitions as expected by views.module (see Views Documentation). 
 *     When proving several filters, it is recommended to use the 'name' 
 *     attribute in order to let the user distinguish between them. If no 'name'
 *     is specified for a filter, the key of the filter will be used instead.
 */
function hook_field_settings($op, $field) {
  switch ($op) {
    case 'form':
      $form = array();
      $form['max_length'] = array(
        '#type' => 'textfield',
        '#title' => t('Maximum length'),
        '#default_value' => $field['max_length'] ? $field['max_length'] : '',
        '#required' => FALSE,
        '#description' => t('The maximum length of the field in characters. Leave blank for an unlimited size.'),
      );
      return $form;

    case 'save':
      return array('text_processing', 'max_length', 'allowed_values');

    case 'database columns':
      $columns = array(
        'value' => array('type' => 'varchar', 'not null' => TRUE, 'default' => "''", 'sortable' => TRUE),
        'format' => array('type' => 'int', 'length' => 10, 'unsigned' => TRUE, 'not null' => TRUE, 'default' => 0),
      );
      if ($field['max_length'] == 0 || $field['max_length'] > 255) {
        $columns['value']['type'] = 'longtext';
      }
      else {
        $columns['value']['length'] = $field['max_length'];
      }
      return $columns;

    case 'filters':
      return array(
        'substring' => array(
          'operator' => 'views_handler_operator_like',
          'handler' => 'views_handler_filter_like',
        ),
        'alpha_order' => array(
          'name' => 'alphabetical order',
          'operator' => 'views_handler_operator_gtlt',
        ),
      );
  }
}

/**
 * Define the behavior of a field type.
 *
 * @param $op
 *   What kind of action is being performed. Possible values:
 *   - "load": The node is about to be loaded from the database. This hook
 *     should be used to load the field.
 *   - "view": The node is about to be presented to the user. The module
 *     should prepare and return an HTML string containing a default
 *     representation of the field.
 *   - "validate": The user has just finished editing the node and is
 *     trying to preview or submit it. This hook can be used to check or
 *     even modify the node. Errors should be set with form_set_error().
 *   - "submit": The user has just finished editing the node and the node has
 *     passed validation. This hook can be used to modify the node.
 *   - "insert": The node is being created (inserted in the database).
 *   - "update": The node is being updated.
 *   - "delete": The node is being deleted.
 * @param &$node
 *   The node the action is being performed on. This argument is passed by
 *   reference for performance only; do not modify it.
 * @param $field
 *   The field the action is being performed on.
 * @param &$node_field
 *   The contents of the field in this node. Changes to this variable will
 *   be saved back to the node object.
 * @return
 *   This varies depending on the operation.
 *   - The "load" operation should return an object containing extra values
 *     to be merged into the node object.
 *   - The "view" operation should return a string containing an HTML
 *     representation of the field data.
 *   - The "insert", "update", "delete", "validate", and "submit" operations
 *     have no return value.
 *
 * In most cases, only "view" and "validate" are relevant operations; the rest
 * have default implementations in content_field() that usually suffice.
 */
function hook_field($op, &$node, $field, &$node_field, $teaser, $page) {
  switch ($op) {
    case 'view':
      if ($field['multiple']) {
        foreach ($node_field as $delta => $item) {
          $node_field[$delta]['view'] = text_field_view($field, $item['value'], $item, $node);
        }
      }
      else {
        $node_field['view'] = text_field_view($field, $node_field['value'], $node_field, $node);
      }

      if ($field['multiple']) {
        $output = '';
        foreach ($node_field as $delta => $item) {
          $output .= '<div class="'. $field['field_name'] .'">'. $item['view'] .'</div>';
        }
        return $output;
      }
      else {
        return '<div class="'. $field['field_name'] .'">'. $node_field['view'] .'</div>';
      }

    case 'validate':
      $allowed_values = explode("\n", $field['allowed_values']);
      $allowed_values = array_map('trim', $allowed_values);
      $allowed_values = array_filter($allowed_values, 'strlen');

      if ($field['multiple']) {
        if (is_array($node_field)) {
          foreach ($node_field as $delta => $item) {
            if ($item['value'] != '') {
              if (count($allowed_values) && !in_array($item['value'], $allowed_values)) {
                form_set_error($field['field_name'], t('Illegal value for %name.', array('%name' => t($field['widget']['label']))));
              }
            }
          }
        }
      }
      else {
        if (isset($node_field['value'])) {
          if ($node_field['value'] != '') {
            if (count($allowed_values) && !in_array($node_field['value'], $allowed_values)) {
              form_set_error($field['field_name'], t('Illegal value for %name.', array('%name' => t($field['widget']['label']))));
            }
          }
        }
      }
      break;
  }
}

/**
 * Prepare an individual item for viewing in a browser.
 *
 * @param $field
 *   The field the action is being performed on.
 * @param $node_field_item
 *   An array, keyed by column, of the data stored for this item in this field.
 *
 * @return
 *   An HTML string containing the formatted item.
 *
 * In a multiple-value field scenario, this function will be called once per
 * value currently stored in the field. This function is also used as the handler
 * for viewing a field in a views.module tabular listing.
 *
 * It is important that this function at the minimum perform security
 * transformations such as running check_plain() or check_markup().
 */
function hook_field_view_item($field, $node_field_item) {
  return check_plain($node_field_item['value']);
}


/**
 * Declare information about a widget.
 *
 * @return
 *   An array keyed by widget name. Each element of the array is an associative
 *   array with these keys and values:
 *   - "label": The human-readable label for the widget.
 *   - "field types": An array of field type names that can be edited using
 *     this widget.
 */
function hook_widget_info() {
  return array(
    'text' => array(
      'label' => 'Text Field',
      'field types' => array('text'),
    ),
  );
}

/**
 * Handle the parameters for a widget.
 *
 * @param $op
 *   The operation to be performed. Possible values:
 *   - "form": Display the widget settings form.
 *   - "validate": Check the widget settings form for errors.
 *   - "save": Declare which pieces of information to save back to the database.
 * @param $widget
 *   The widget on which the operation is to be performed.
 * @return
 *   This varies depending on the operation.
 *   - The "form" operation should return an array of form elements to add to
 *     the settings page.
 *   - The "validate" operation has no return value. Use form_set_error().
 *   - The "save" operation should return an array of names of form elements to
 *     be saved in the database.
 */
function hook_widget_settings($op, $widget) {
  switch ($op) {
    case 'form':
      $form = array();
      $form['rows'] = array(
        '#type' => 'textfield',
        '#title' => t('Rows'),
        '#default_value' => $widget['rows'] ? $widget['rows'] : 1,
        '#required' => TRUE,
      );
      return $form;

    case 'validate':
      if (!is_numeric($widget['rows']) || intval($widget['rows']) != $widget['rows'] || $widget['rows'] <= 0) {
        form_set_error('rows', t('"Rows" must be a positive integer.'));
      }
      break;

    case 'save':
      return array('rows');
  }
}

/**
 * Define the behavior of a widget.
 *
 * @param $op
 *   What kind of action is being performed. Possible values:
 *   - "prepare form values": The editing form will be displayed. The widget
 *     should perform any conversion necessary from the field's native storage
 *     format into the storage used for the form. Convention dictates that the
 *     widget's version of the data should be stored beginning with "default".
 *   - "form": The node is being edited, and a form should be prepared for
 *     display to the user.
 *   - "validate": The user has just finished editing the node and is
 *     trying to preview or submit it. This hook can be used to check or
 *     even modify the node. Errors should be set with form_set_error().
 *   - "process form values": The inverse of the prepare operation. The widget
 *     should convert the data back to the field's native format.
 *   - "submit": The user has just finished editing the node and the node has
 *     passed validation. This hook can be used to modify the node.
 * @param &$node
 *   The node the action is being performed on. This argument is passed by
 *   reference for performance only; do not modify it.
 * @param $field
 *   The field the action is being performed on.
 * @param &$node_field
 *   The contents of the field in this node. Changes to this variable will
 *   be saved back to the node object.
 * @return
 *   This varies depending on the operation.
 *   - The "form" operation should return an array of form elements to display.
 *   - Other operations have no return value.
 */
function hook_widget($op, &$node, $field, &$node_field) {
  switch ($op) {
    case 'prepare form values':
      if ($field['multiple']) {
        $node_field_transposed = content_transpose_array_rows_cols($node_field);
        $node_field['default nids'] = $node_field_transposed['nid'];
      }
      else {
        $node_field['default nids'] = array($node_field['nid']);
      }
      break;

    case 'form':
      $form = array();

      $form[$field['field_name']] = array('#tree' => TRUE);
      $form[$field['field_name']]['nids'] = array(
        '#type' => 'select',
        '#title' => t($field['widget']['label']),
        '#default_value' => $node_field['default nids'],
        '#multiple' => $field['multiple'],
        '#options' => _nodereference_potential_references($field),
        '#required' => $field['required'],
        '#description' => $field['widget']['description'],
      );
      return $form;

    case 'process form values':
      if ($field['multiple']) {
        $node_field = content_transpose_array_rows_cols(array('nid' => $node_field['nids']));
      }
      else {
        $node_field['nid'] = is_array($node_field['nids']) ? reset($node_field['nids']) : $node_field['nids'];
      }
      break;
  }
}



/**
 * @} End of "addtogroup hooks".
 */
