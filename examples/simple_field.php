<?php
// $Id$

/**
 * @file
 * SIMPLE EXAMPLE.
 * This is similar to to example_field but creates only a single widget
 * formatted in the traditional manner, not using hook_elements. This
 * example also omits all optional parts of the field module to create
 * a simpler example for developers who don't wish to create re-usable
 * elements.
 *
 *
 * These hooks are defined by field modules, modules that define a new kind
 * of field for insertion in a content type.
 *
 * Field hooks are typically called by content.module using _content_field_invoke().
 *
 * Widget module hooks are also defined here; the two go hand-in-hand, often in
 * the same module (though they are independent).
 *
 * Widget hooks are typically called by content.module when it creates the field
 * form elements in the node form using hook_form_alter().
 */

/**
 * @addtogroup install
 * @{
 */
/**
 * Implementation of hook_content_notify().
 *
 * This hook should be implemented inside hook_install(), hook_uninstall(),
 * hook_enable() and hook_disable(), and is used to notify the content
 * module when a field module is added or removed so it can respond
 * appropriately. One use of this hook is to allow the content module
 * to remove fields and field data created by this module when the
 * module is uninstalled.
 *
 * The recommended location for these hooks is in the module's .install file.
 */
/**
 * Implementation of hook_install().
 */
function text_install() {
  content_notify('install', 'text');
}

/**
 * Implementation of hook_uninstall().
 */
function text_uninstall() {
  content_notify('uninstall', 'text');
}

/**
 * Implementation of hook_enable().
 *
 * Notify content module when this module is enabled.
 */
function text_enable() {
  content_notify('enable', 'text');
}

/**
 * Implementation of hook_disable().
 *
 * Notify content module when this module is disabled.
 */
function text_disable() {
  content_notify('disable', 'text');
}
/**
 * @} End of "addtogroup install".
 */

/**
 * @addtogroup hooks
 * @{
 */
/**
 * Implementation of hook_theme().
 */
function text_theme() {
  return array(
    'text_textarea' => array(
      'arguments' => array('element' => NULL),
    ),
    'text_textfield' => array(
      'arguments' => array('element' => NULL),
    ),
  );
}

/**
 * Implementation of hook_field_info().
 *
 * Here we indicate that the content module will use its default
 * handling for the view of this field.
 *
 * Callbacks can be omitted if default handing is used.
 * They're included here just so this module can be used
 * as an example for custom modules that might do things
 * differently.
 *
 * If your module will provide its own Views tables or arguments,
 * change CONTENT_CALLBACK_DEFAULT to CONTENT_CALLBACK_CUSTOM.
 */
function text_field_info() {
  return array(
    'text' => array(
      'label' => 'Text',
      ),
    );
}

/**
 * Implementation of hook_field_settings().
 *
 * Handle the settings for a field.
 *
 * @param $op
 *   The operation to be performed. Possible values:
 *   - "form": Display the field settings form.
 *   - "validate": Check the field settings form for errors.
 *   - "save": Declare which fields to save back to the database.
 *   - "database columns": Declare the columns that content.module should create
 *     and manage on behalf of the field. If the field module wishes to handle
 *     its own database storage, this should be omitted.
 *   - "filters": Declare the Views filters available for the field.
 *     (this is used in CCK's default Views tables definition)
 *     They always apply to the first column listed in the "database columns"
 *     array.
 * @param $field
 *   The field on which the operation is to be performed.
 * @return
 *   This varies depending on the operation.
 *   - "form": an array of form elements to add to
 *     the settings page.
 *   - "validate": no return value. Use form_set_error().
 *   - "save": an array of names of form elements to
 *     be saved in the database.
 *   - "database columns": an array keyed by column name, with arrays of column
 *     information as values. This column information must include "type", the
 *     MySQL data type of the column, and may also include a "sortable" parameter
 *     to indicate to views.module that the column contains ordered information.
 *     TODO : Details of other information that can be passed to the database layer can
 *     be found in the API for the Schema API.
 *   - "filters": an array of 'filters' definitions as expected by views.module
 *     (see Views Documentation).
 *     When providing several filters, it is recommended to use the 'name'
 *     attribute in order to let the user distinguish between them. If no 'name'
 *     is specified for a filter, the key of the filter will be used instead.
 */
function text_field_settings($op, $field) {
  switch ($op) {
    case 'form':
      $form = array();
      $form['max_length'] = array(
        '#type' => 'textfield',
        '#title' => t('Maximum length'),
        '#default_value' => is_numeric($field['max_length']) ? $field['max_length'] : '',
        '#required' => FALSE,
        '#description' => t('The maximum length of the field in characters. Leave blank for an unlimited size.'),
      );
      return $form;

    case 'save':
      return array('max_length');

    case 'database columns':
      $columns['value'] = array('type' => 'varchar', 'length' => $field['max_length'], 'not null' => FALSE, 'sortable' => TRUE);
      return $columns;

    case 'filters':
      return array(
        'like' => array(
          'operator' => 'views_handler_operator_like',
          'handler' => 'views_handler_filter_like',
        ),
      );
      break;
  }
}

/**
 * Implementation of hook_field().
 *
 * Define the behavior of a field type.
 *
 * @param $op
 *   What kind of action is being performed. Possible values:
 *   - "load": The node is about to be loaded from the database. This hook
 *     should be used to load the field.
 *   - "validate": The user has just finished editing the node and is
 *     trying to preview or submit it. This hook can be used to check or
 *     even modify the node. Errors should be set with form_set_error().
 *   - "presave": The user has just finished editing the node and the node has
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
 *   - The "insert", "update", "delete", "validate", and "presave" operations
 *     have no return value.
 *
 * In most cases, only "validate" operations is relevant ; the rest
 * have default implementations in content_field() that usually suffice.
 */
function text_field($op, &$node, $field, &$items, $teaser, $page) {
  switch ($op) {
    case 'validate':
      if (!empty($field['max_length'])) {
        foreach ($items as $delta => $data) {
          $error_field = $field['field_name'] .']['. $delta .'][value';
          if (strlen($data['value']) > $field['max_length']) {
            form_set_error($error_field, t('%label is longer than %max characters.', array('%label' => $field['widget']['label'], '%max' => $field['max_length'])));
          }
        }
      }
      break;
  }
}

/**
 * Implementation of hook_content_is_empty().
 *
 * NEW REQUIRED HOOK!
 *
 * This function tells the content module whether or not to consider
 * the $item to be empty. This is used by the content module
 * to remove empty, non-required values before saving them.
 */
function text_content_is_empty($item, $field) {
  if (empty($item['value'])) {
    return TRUE;
  }
  return FALSE;
}

/**
 * Implementation of hook_field_formatter_info().
 */
function text_field_formatter_info() {
  return array(
    'default' => array(
      'label' => 'Default',
      'field types' => array('text'),
    ),
    'plain' => array(
      'label' => 'Plain text',
      'field types' => array('text'),
    ),
  );
}

/**
 * Implementation of hook_field_formatter().
 *
 * Prepare an individual item for viewing in a browser.
 *
 * @param $field
 *   The field the action is being performed on.
 * @param $item
 *   An array, keyed by column, of the data stored for this item in this field.
 * @param $formatter
 *   The name of the formatter being used to display the field.
 * @param $node
 *   The node object, for context. Will be NULL in some cases.
 *   Warning : when displaying field retrieved by Views, $node will not
 *   be a "full-fledged" node object, but an object containg the data returned
 *   by the Views query (at least nid, vid, changed)
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
function text_field_formatter($field, $item, $formatter, $node) {
  if (!isset($item['value'])) {
    return '';
  }

  switch ($formatter) {
    case 'plain':
      $text = strip_tags($item['value']);
      break;

    case 'default':
      $text = $item['value'];
  }

  return check_plain($text);
}


/**
 * Implementation of hook_widget_info().
 *
 * Here we indicate that the content module will handle
 * the default value and multiple values for these widgets.
 *
 * Callbacks can be omitted if default handing is used.
 * They're included here just so this module can be used
 * as an example for custom modules that might do things
 * differently.
 */
function text_widget_info() {
  return array(
    'text_textfield' => array(
      'label' => 'Text Field',
      'field types' => array('text'),
    ),
  );
}

/**
 * Implementation of hook_widget_settings().
 *
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
 *   - "form": an array of form elements to add to the settings page.
 *   - "validate": no return value. Use form_set_error().
 *   - "save": an array of names of form elements to be saved in the database.
 */
function text_widget_settings($op, $widget) {
  switch ($op) {
    case 'form':
      $form = array();
      $form['rows'] = array(
        '#type' => 'textfield',
        '#title' => t('Rows'),
        '#default_value' => is_numeric($widget['rows']) ? $widget['rows'] : 5,
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
 * Implementation of hook_widget().
 *
 * Attach a single form element to the form. It will be built out and
 * validated in the callback(s) listed in hook_elements. We build it
 * out in the callbacks rather than here in hook_widget so it can be
 * plugged into any module that can provide it with valid
 * $field information.
 *
 * Content module will set the weight, field name and delta values
 * for each form element. This is a change from earlier CCK versions
 * where the widget managed its own multiple values.
 *
 * If there are multiple values for this field, the content module will
 * call this function as many times as needed.
 *
 * @param $form
 *   the entire form array, $form['#node'] holds node information
 * @param $form_state
 *   the form_state, $form_state['values'][$field['field_name']]
 *   holds the field's form values.
 * @param $field
 *   the field array
 * @param $items
 *   array of default values for this field
 * @param $delta
 *   the order of this item in the array of subelements (0, 1, 2, etc)
 *
 * @return
 *   the form item for a single element for this field
 */
function text_widget(&$form, &$form_state, $field, $items, $delta = 0) {
  $field_key   = $element['#columns'][0];
  $element[$field_key] = array(
    '#type' => 'textarea',
    '#title' => t($field['widget']['label']),
    '#description' => t($field['widget']['description']),
    '#required' => $element['#required'],
    '#default_value' => isset($items[$delta]) ? $items[$delta] : NULL,
    '#rows' => !empty($field['widget']['rows']) ? $field['widget']['rows'] : 1,
    '#weight' => 0,
  );
  return $element;
}

/**
 * @} End of "addtogroup hooks".
 */
