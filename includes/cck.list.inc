<?php
// $Id$

/**
 * A custom function to return allowed values from PHP code.
 */
function cck_list_allowed_values_php($field) {
  $allowed_values = array();
  $php = cck_field_get_setting('allowed_values_php', 'field', $field);
  if (!empty($php)) {
    ob_start();
    $result = eval($php);
    if (is_array($result)) {
      $allowed_values = $result;
    }
    ob_end_clean();
  }
  return $allowed_values;  
}

/**
 * Implementation of hook_field_settings_form()
 * on behalf of core List module.
 */
function list_field_settings_form($field, $instance) {
  $form = array(
    '#element_validate' => array('list_field_settings_form_validate'),
    );
  
  $defaults = field_info_field_settings($field['type']);
  $settings = array_merge($defaults, $field['settings']);
  $settings['allowed_values_php'] = cck_field_get_setting('allowed_values_php', 'field', $field);
  $settings['allowed_values_function'] = !empty($field['allowed_values_function']) && $field['allowed_values_function'] != 'cck_list_allowed_values_php' ? $field['allowed_values_function'] : '';
  $form['allowed_values'] = array(
    '#type' => 'textarea',
    '#title' => t('Allowed values list'),
    '#default_value' => $settings['allowed_values'],
    '#required' => FALSE,
    '#rows' => 10,
    '#description' => t('The possible values this field can contain. Enter one value per line, in the format key|label. The key is the value that will be stored in the database, and must be a %type value. The label is optional, and the key will be used as the label if no label is specified.<br />Allowed HTML tags in labels: @tags', array('%type' => $field['type'] == 'list_text' ? 'text' : 'numeric', '@tags' => _field_filter_xss_display_allowed_tags())),
  );
  $form['allowed_values_function'] = array(
    '#type' => 'textfield',
    '#title' => t('Allowed values function'),
    '#default_value' => $settings['allowed_values_function'],
    '#description' => t('The name of a function that will return the allowed values list. If this field is filled out, the array returned by this code will override the allowed values list above.'),
  );
  $form['advanced_options'] = array(
    '#type' => 'fieldset',
    '#title' => t('PHP code for allowed values'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($settings['allowed_values_php']),
  );
  if (user_access('Use PHP input for field settings (dangerous - grant with care)')) {
    $form['advanced_options']['allowed_values_php'] = array(
      '#type' => 'textarea',
      '#title' => t('Code'),
      '#default_value' => $settings['allowed_values_php'],
      '#rows' => 6,
      '#description' => t("Advanced usage only: PHP code that returns a keyed array of allowed values. Should not include &lt;?php ?&gt; delimiters. If this field is filled out, the array returned by this code will override the allowed values list, and the allowed values function will be set to 'cck_list_allowed_values_php' to execute this custom code. <strong>The option to embed PHP code in the field definition is provided for backwards compatibility and could be deprecated in the future. It is strongly recommended that you move this code to a custom function in a custom module and simply identify the custom function in the box above!</strong>"),
    );
  }
  else {
    $form['advanced_options']['markup_allowed_values_php'] = array(
      '#type' => 'item',
      '#title' => t('Code'),
      '#value' => !empty($settings['allowed_values_php']) ? '<code>'. check_plain($settings['allowed_values_php']) .'</code>' : t('&lt;none&gt;'),
      '#description' => empty($settings['allowed_values_php']) ? t("You're not allowed to input PHP code.") : t('This PHP code was set by an administrator and will override the allowed values list and allowed values functions shown above.'),
    );
  }
  return $form;
}

/**
 * Handle Allowed values PHP code.
 */
function list_field_settings_form_validate($form, &$form_state) {
  $form_values = $form_state['values'];
  $field = $form_values['field'];
  $field_name = $field['field_name'];
  $option = $form_values['field']['settings']['advanced_options']['allowed_values_php'];
  cck_field_set_setting('allowed_values_php', 'field', $option, $field);
  
  if (empty($form_values['allowed_values_function'])) {
    form_set_value($form['allowed_values_function'], 'cck_list_allowed_values_php', $form_state);
  }
  form_set_value($form['advanced_options'], NULL, $form_state);    
}
