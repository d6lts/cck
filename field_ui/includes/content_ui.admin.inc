<?php
// $Id$

/**
 * @file
 * Administrative interface for content type creation.
 */

/**
 * Menu callback; lists all defined fields for quick reference.
 */
function _content_admin_type_fields() {
  $fields = content_fields();

  $header = array(t('Field name'), t('Field type'), t('Used in'));
  $rows = array();
  foreach ($fields as $field) {
    $row = array();
    $row[] = $field['field_name'];
    $row[] = $field['type'];

    $types = array();
    $result = db_query("SELECT nt.name, nt.type FROM {". content_instance_tablename() ."} nfi ".
    "LEFT JOIN {node_type} nt ON nt.type = nfi.type_name ".
    "WHERE nfi.field_name = '%s' ".
    // Keep disabled modules out of table.
    "AND nfi.widget_active = 1 ".
    "ORDER BY nt.name ASC", $field['field_name']);
    while ($type = db_fetch_array($result)) {
      $content_type = content_types($type['type']);
      $types[] = l($type['name'], 'admin/build/node-type/'. $content_type['url_str'] .'/fields');
    }
    $row[] = implode(', ', $types);

    $rows[] = $row;
  }
  if (empty($rows)) {
    $output = t('No fields have been defined for any content type yet.');
  }
  else {
    $output = theme('table', $header, $rows);
  }
  return $output;
}

/**
 * Menu callback; listing of fields for a content type.
 *
 * Allows fields to be reordered and nested in fieldgroups using
 * JS drag-n-drop. Non-CCK form elements can also be moved around.
 */
function content_admin_field_overview_form(&$form_state, $type_name) {
  // When displaying the form, make sure the list of 'extra' fields
  // is up-to-date.
  if (empty($form_state['post'])) {
    content_clear_type_cache();
  }

  // Gather type information.
  $type = content_types($type_name);
  $fields = $type['instances'];
  if (empty($fields)) {
    drupal_set_message(t('There are no fields configured for this content type. You can !link.', array(
      '!link' => l(t('Add a new field'), str_replace('/fields', '/add_field', $_GET['q'])))), 'warning');
    return array();
  }

  $extra = $type['extra'];
  $groups = $group_options = array();
  if (module_exists('fieldgroup')) {
    $groups = fieldgroup_groups($type['type']);
    $group_options = _fieldgroup_groups_label($type['type']);
  }


  // Rows in this table are essentially nested, but for the simplicity of
  // theme and submit functions, we keep them in a flat array, and use a
  // $dummy render structure to figure the right display order.
  $dummy = array();

  $form = array(
    '#tree' => TRUE,
    '#type_name' => $type['type'],
    '#fields' => array_keys($fields),
    '#groups' => array_keys($groups),
    '#extra' => array_keys($extra),
    '#order' => array(),
    '#prefix' => '<p>'. t('To change the order of a field, grab a drag-and-drop handle under the Label column and drag the field to a new location in the list. (Grab a handle by clicking and holding the mouse while hovering over a handle icon.) Remember that your changes will not be saved until you click the Save button at the bottom of the page.') .'</p>',
  );

  // Fields.
  foreach ($fields as $name => $field) {
    $weight = $field['widget']['weight'];
    $form[$name] = array(
      'human_name' => array('#value' => $field['widget']['label']),
      'name' => array('#value' => $field['field_name']),
      'type' => array('#value' => $field['type']),
      'configure' => array('#value' => l(t('Configure'), 'admin/build/node-type/'. $type['url_str'] .'/fields/'. $field['field_name'] .'/edit')),
      'remove' => array('#value' => l(t('Remove'), 'admin/build/node-type/'. $type['url_str'] .'/fields/'. $field['field_name'] .'/remove')),
      'weight' => array('#type' => 'textfield', '#default_value' => $weight),
      'parent' => array('#type' => 'select', '#options' => $group_options, '#default_value' => ''),
      'hidden_name' => array('#type' => 'hidden', '#default_value' => $field['field_name']),
      '#leaf' => TRUE,
    );
    $dummy[$name] = array('#weight' => $weight, '#value' => $name .' ');
  }
  // Groups.
  foreach ($groups as $name => $group) {
    $weight = $group['weight'];
    $form[$name] = array(
      'human_name' => array('#value' => $group['label']),
      'name' => array('#value' => $group['group_name']),
      'type' => array(),
      'configure' => array('#value' => l(t('Configure'), 'admin/build/node-type/'. $type['url_str'] .'/groups/'. $group['group_name'] .'/edit')),
      'remove' => array('#value' => l(t('Remove'), 'admin/build/node-type/'. $type['url_str'] .'/groups/'. $group['group_name'] .'/remove')),
      'weight' => array('#type' => 'textfield', '#default_value' => $weight),
      'parent' => array('#type' => 'hidden', '#default_value' => ''),
      'hidden_name' => array('#type' => 'hidden', '#default_value' => $group['group_name']),
      '#root' => TRUE,
    );
    $dummy[$name] = array('#weight' => $weight, '#value' => $name .' ');
    // Adjust child fields rows.
    foreach ($group['fields'] as $field_name => $field) {
      $form[$field_name]['#depth'] = 1;
      $form[$field_name]['parent']['#default_value'] = $name;
      $dummy[$name][$field_name] = $dummy[$field_name];
      unset($dummy[$field_name]);
    }
  }
  // Non-CCK 'fields'.
  foreach ($extra as $name => $label) {
    $weight = $extra[$name]['weight'];
    $form[$name] = array(
      'human_name' => array('#value' => $extra[$name]['label']),
      'name' => array(),
      'type' => array(),
      'configure' => array(),
      'remove' => array(),
      'weight' => array('#type' => 'textfield', '#default_value' => $weight),
      'parent' => array('#type' => 'hidden', '#default_value' => ''),
      'hidden_name' => array('#type' => 'hidden', '#default_value' => $name),
      '#leaf' => TRUE,
      '#root' => TRUE,
      '#disabled' => TRUE,
    );
    $dummy[$name] = array('#weight' => $weight, '#value' => $name .' ');
  }

  // Let drupal_render figure out the right order for the rows.
  $form['#order'] = explode(' ', trim(drupal_render($dummy)));

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
  return $form;
}

/**
 *  Theme the field overview table.
 */
function theme_content_admin_field_overview_form($form) {
  if (empty($form['#order'])) {
    return;
  }
  $header = array(t('Label'), t('Name'), t('Type'), t('Weight'), array('data' => t('Operations'), 'colspan' => 2));
  $rows = array();

  foreach ($form['#order'] as $key) {
    $row = array();
    $element = &$form[$key];
    $element['weight']['#attributes']['class'] = 'field-weight';
    $element['parent']['#attributes']['class'] = 'group-parent';
    $element['hidden_name']['#attributes']['class'] = 'field-name';
    if (in_array($key, $form['#groups'])) {
      $element['human_name']['#prefix'] = '<strong>';
      $element['human_name']['#suffix'] = '</strong>';
    }
    $row[] = theme('indentation', isset($element['#depth']) ? $element['#depth'] : 0) . drupal_render($element['human_name']);
    $row[] = drupal_render($element['name']);
    $row[] = drupal_render($element['type']);
    $row[] = drupal_render($element['weight']) . drupal_render($element['parent']) . drupal_render($element['hidden_name']);
    $row[] = drupal_render($element['configure']);
    $row[] = drupal_render($element['remove']);
    $class = 'draggable';
    $class .= isset($element['#disabled']) ? ' menu-disabled' : '';
    $class .= isset($element['#leaf']) ? ' tabledrag-leaf' : '';
    $class .= isset($element['#root']) ? ' tabledrag-root' : '';
    $rows[] = array('data' => $row, 'class' => $class);
  }

  $output  = theme('table', $header, $rows, array('id' => 'content-field-overview'));

  drupal_add_tabledrag('content-field-overview', 'match', 'parent', 'group-parent', 'group-parent', 'field-name', TRUE, 1);
  drupal_add_tabledrag('content-field-overview', 'order', 'sibling', 'field-weight');

  // Hide the 'Save' button, show it when fields are swapped.
  $form['submit']['#attributes']['class'] = 'content-admin-field-overview-submit';
  drupal_add_js('if (Drupal.jsEnabled) { $(document).ready(function() { $(".content-admin-field-overview-submit").hide(); }); }', 'inline');
  drupal_add_js(drupal_get_path('module', 'content') .'/content.js');

  $output .= drupal_render($form);

  return $output;
}

function content_admin_field_overview_form_submit($form, &$form_state) {
  $form_values = $form_state['values'];

  // Update field weights.
  $extra = array();
  foreach ($form_values as $key => $values) {
    // Groups are handled in fieldgroup_content_overview_form_submit().
    if (in_array($key, $form['#fields'])) {
      db_query("UPDATE {". content_instance_tablename() ."} SET weight = %d WHERE type_name = '%s' AND field_name = '%s'",
        $values['weight'], $form['#type_name'], $key);
    }
    elseif (in_array($key, $form['#extra'])) {
      $extra[$key] = $values['weight'];
    }
  }

  if ($extra) {
    variable_set('content_extra_weights_'. $form['#type_name'], $extra);
  }
  else {
    variable_del('content_extra_weights_'. $form['#type_name']);
  }

  content_clear_type_cache();
}

/**
 * Menu callback; presents a listing of fields display settings for a content type.
 *
 * Form includes form widgets to select which fields appear for teaser, full node
 * and how the field labels should be rendered.
 */
function content_admin_display_overview_form(&$form_state, $type_name, $contexts_selector = CONTENT_CONTEXTS_SIMPLE) {
  // Gather type information.
  $type = content_types($type_name);
  $field_types = _content_field_types(); // TODO : needed ?
  $fields = $type['fields'];
  if (empty($fields)) {
    drupal_set_message(t('There are no fields configured for this content type. You can !link.', array(
      '!link' => l(t('Add a new field'), str_replace('/fields', '/add_field', $_GET['q'])))), 'warning');
    return array();
  }

  $groups = $group_options = array();
  if (module_exists('fieldgroup')) {
    $groups = fieldgroup_groups($type['type']);
    $group_options = _fieldgroup_groups_label($type['type']);
  }
  $contexts = _content_admin_display_contexts($contexts_selector);

  // Rows in this table are essentially nested, but for the simplicity of
  // theme and submit functions, we keep them in a flat array, and use a
  // $dummy render structure to figure the right display order.
  $dummy = array();

  $form = array(
    '#tree' => TRUE,
    '#type_name' => $type['type'],
    '#fields' => array_keys($fields),
    '#groups' => array_keys($groups),
    '#contexts' => $contexts_selector,
    '#order' => array(),
  );

  // Fields.
  $label_options = array(
    'above' => t('Above'),
    'inline' => t('Inline'),
    'hidden' => t('<Hidden>'),
  );
  foreach ($fields as $name => $field) {
    $field_type = $field_types[$field['type']];
    $defaults = $field['display_settings'];
    $weight = $field['widget']['weight'];

    $form[$name] = array(
      'human_name' => array('#value' => $field['widget']['label']), // OK
    );

    // Label
    if ($contexts_selector == CONTENT_CONTEXTS_SIMPLE) {
      $form[$name]['label']['format'] = array(
        '#type' => 'select',
        '#options' => $label_options,
        '#default_value' => isset($defaults['label']['format']) ? $defaults['label']['format'] : 'above',
      );
    }

    // Formatters.
    $options = array();
    foreach ($field_type['formatters'] as $formatter_name => $formatter_info) {
      $options[$formatter_name] = $formatter_info['label'];
    }
    $options['hidden'] = t('<Hidden>');
    foreach ($contexts as $key => $title) {
      $form[$name][$key]['format'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => isset($defaults[$key]['format']) ? $defaults[$key]['format'] : 'default',
      );
    }

    $dummy[$name] = array('#weight' => $weight, '#value' => $name .' ');
  }

  // Groups.
  $label_options = array(
    'above' => t('Above'),
    'hidden' => t('<Hidden>'),
  );
  $options = array(
    'no_style' => t('no styling'),
    'simple' => t('simple'),
    'fieldset' => t('fieldset'),
    'fieldset_collapsible' => t('fieldset - collapsible'),
    'fieldset_collapsed' => t('fieldset - collapsed'),
    'hidden' => t('<Hidden>'),
  );
  foreach ($groups as $name => $group) {
    $defaults = $group['settings']['display'];
    $weight = $group['weight'];

    $form[$name] = array(
      'human_name' => array('#value' => $group['label']),
    );
    $form[$name]['label'] = array(
      '#type' => 'select',
      '#options' => $label_options,
      '#default_value' => isset($defaults['label']) ? $defaults['label'] : 'above',
    );
    foreach ($contexts as $key => $title) {
      $form[$name][$key]['format'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#default_value' => isset($defaults[$key]) ? $defaults[$key] : 'fieldset',
      );
    }
    $dummy[$name] = array('#weight' => $weight, '#value' => $name .' ');
    // Adjust child fields rows.
    foreach ($group['fields'] as $field_name => $field) {
      $form[$field_name]['#depth'] = 1;
      $dummy[$name][$field_name] = $dummy[$field_name];
      unset($dummy[$field_name]);
    }
  }

  // Let drupal_render figure out the right order for the rows.
  $form['#order'] = explode(' ', trim(drupal_render($dummy)));

  $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
  return $form;
}

/**
 *  Theme the display overview table.
 */
function theme_content_admin_display_overview_form($form) {
  $output = '';
  if (isset($form['#order'])) {
    $contexts = _content_admin_display_contexts($form['#contexts']);
    $header = array(t('Field'), t('Label'));
    foreach ($contexts as $key => $title) {
      $header[] = $title;
    }
    $rows = array();

    foreach ($form['#order'] as $key) {
      $row = array();
      $element = &$form[$key];
      if (in_array($key, $form['#groups'])) {
        $element['human_name']['#prefix'] = '<strong>';
        $element['human_name']['#suffix'] = '</strong>';
      }
      $row[] = theme('indentation', isset($element['#depth']) ? $element['#depth'] : 0) . drupal_render($element['human_name']);
      $row[] = drupal_render($element['label']);
      foreach ($contexts as $context => $title) {
        $row[] = drupal_render($element[$context]['format']);
      }
      $rows[] = $row;
    }

    $output .= theme('table', $header, $rows);
  }

  $output .= drupal_render($form);
  return $output;
}

/**
 * Submit handler for the display overview form.
 */
function content_admin_display_overview_form_submit($form, &$form_state) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];
  $fields = content_fields();
  foreach ($form_values as $key => $values) {
    // Groups are handled in fieldgroup_display_overview_form_submit().
    if (in_array($key, $form['#fields'])) {
      $field = content_fields($key, $form['#type_name']);
      // We have numeric keys here, so we can't use array_merge.
      $field['display_settings'] = $values + $field['display_settings'];
      //$field = content_field_instance_collapse($field);
      field_update_instance($field);
    }
    content_clear_type_cache();
  }

  drupal_set_message(t('Your settings have been saved.'));
}

/**
 * Menu callback; presents the form for adding a new field.
 */
function _content_admin_field_add($type_name) {
  // make sure the old field list gets cleared before creating the new one
  if (!isset($_POST['edit'])) {
    content_clear_type_cache();
  }
  $output = drupal_get_form('_content_admin_field_add_existing', $type_name);
  $output .= drupal_get_form('_content_admin_field_add_new', $type_name);
  return $output;
}

function _content_admin_field_add_existing(&$form_state, $type_name) {
  $output = '';
  $type = content_types($type_name);
  $fields = content_fields();
  $form = array();

  $options = array();
  foreach ($fields as $field) {
    if (!isset($type['fields'][$field['field_name']]))
    $options[$field['field_name']] = t($field['widget']['label']) .' ('. $field['field_name'] .')';
  }
  if ($options) {
    $form['existing'] = array(
      '#type' => 'fieldset',
      '#title' => t('Add existing field'),
    );
    $form['existing']['field_name'] = array(
      '#type' => 'select',
      '#required' => TRUE,
      '#options' => $options,
    );
    $form['existing']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Add field'),
    );
    $form['existing']['type_name'] = array(
      '#type' => 'value',
      '#value' => $type_name,
    );
  }
  return $form;
}

function _content_admin_field_add_new(&$form_state, $type_name, $new_field_name = '') {
  $field_types = _content_field_types();
  $widget_types = _content_widget_types();
  $form = array();
  $field_type_options = array();
  foreach ($field_types as $field_name => $field_type) {
    foreach ($widget_types as $widget_name => $widget_type) {
      if (in_array($field_name, $widget_type['field types'])) {
        $field_type_options[$field_name .'-'. $widget_name] = $widget_type['label'];
      }
    }
  }
  if (count($field_type_options) > 0) {
    $form['new'] = array(
      '#type' => 'fieldset',
      '#title' => t('Create new field'),
    );
    $form['new']['field']['field_name'] = array(
      '#title' => t('Field name'),
      '#type' => 'textfield',
      '#default_value' => $new_field_name,
      '#field_prefix' => 'field_',
      '#description' => t("The machine-readable name of the field. This name cannot be changed later! The name will be prefixed with 'field_' and can include lowercase unaccented letters, numbers, and underscores. You'll be able to choose a human-readable label for the field on next page."),
      '#required' => TRUE,
    );
    $form['new']['field_widget_type'] = array(
      '#type' => 'radios',
      '#title' => t('Field type'),
      '#required' => TRUE,
      '#options' => $field_type_options,
      '#default_value' => NULL,
      '#theme' => 'content_admin_field_add_new_field_widget_type',
    );

    $form['new']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Continue'),
    );
    $form['new']['type_name'] = array(
      '#type' => 'value',
      '#value' => $type_name,
    );
  }
  else {
    drupal_set_message(t('No field modules are enabled. You need to <a href="!modules_url">enable one</a>, such as text.module, before you can add new fields.', array('!modules_url' => url('admin/build/modules'))), 'error');
  }

  return $form;
}

function theme_content_admin_field_add_new_field_widget_type($form) {
  $field_types = _content_field_types();
  $widget_types = _content_widget_types();
  $output = '';
  $output .= '<p>'. t('Choose the type of value to store and an input method from the list below.') .'</p>';
  $output .= '<dl>';
  foreach ($field_types as $field_name => $field_type) {
    $output .= '<dt>'. $field_type['label'] .'</dt>';
    $output .= '<dd><em>'. $field_type['description'] .'</em></dd>';
    foreach ($widget_types as $widget_name => $widget_type) {
      if (in_array($field_name, $widget_type['field types'])) {
        $output .= '<dd>'. drupal_render($form[$field_name .'-'. $widget_name]) .'</dd>';
      }
    }
  }
  $output .= '</dl>';

  return $output;
}

/**
 * Add an existing field to a content type.
 */
function _content_admin_field_add_existing_submit($form, &$form_state) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];

  if (field_create_instance($form_values)) {
    drupal_set_message(t('Added field %label.', array(
      '%label' => $form_values['field_name'])));  }
  else {
    drupal_set_message(t('There was a problem adding field %label.', array(
      '%label' => $form_values['field_name'])));
  }
  $type = content_types($form_values['type_name']);
  $form_state['redirect'] = 'admin/build/node-type/'. $type['url_str'] .'/fields';
}

/**
 *  Field name validation.
 */
function _content_admin_field_add_new_validate($form, &$form_state) {
  // Add the 'field_' prefix.
  form_set_value($form['new']['field']['field_name'], 'field_'. $form_state['values']['field_name'], $form_state);

  $form_values = $form_state['values'];
  if (!preg_match('!^field_[a-z0-9_]+$!', $form_values['field_name'])) {
    form_set_error('field_name', t('The field name %field_name is invalid. The name must include only lowercase unaccentuated letters, numbers, and underscores.', array(
      '%field_name' => $form_values['field_name'])));
  }
  if (array_key_exists($form_values['field_name'], content_fields())) {
    form_set_error('field_name', t('The field name %field_name already exists.', array(
      '%field_name' => $form_values['field_name'])));
  }
  elseif ($form_values['field_name'] == 'field_instance') {
    form_set_error('field_name', t("The name 'field_instance' is a reserved name."));
  }
}

/**
 * Create a new field for a content type.
 */
function _content_admin_field_add_new_submit($form, &$form_state) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];

  $field_widget_type = explode('-', $form_values['field_widget_type']);
  $form_values['type'] = $field_widget_type[0];
  $form_values['widget_type'] = $field_widget_type[1];
  unset($form_values['field_widget_type']);

  // Populate a new instance array.
  $instance = field_instance_new($form_values['field_name'], $form_values['type_name'], $form_values['widget_type']);

  // Check if we're creating the first instance of a new field.
  $field = field_instance_get_field($instance);
  if ($field == NULL) {
    $field = field_new($form_values['field_name'], $form_values['type']);
    field_create_field($field);
  }

  if (field_create_instance($instance)) {
    drupal_set_message(t('Created field %label.', array(
      '%label' => $form_values['field_name'])));
  }
  else {
    drupal_set_message(t('There was a problem creating field %label.', array(
      '%label' => $form_values['field_name'])));
  }

  $type = content_types($form_values['type_name']);
  $form_state['redirect'] = 'admin/build/node-type/'. $type['url_str'] .'/fields/'. $form_values['field_name'];
}

/**
 * Menu callback; present a form for removing a field from a content type.
 */
function _content_admin_field_remove(&$form_state, $type_name, $field_name) {
  $type = content_types($type_name);
  $field = $type['fields'][$field_name];

  $form = array();
  $form['type_name'] = array(
    '#type' => 'value',
    '#value' => $type_name,
  );
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field_name,
  );

  $output = confirm_form($form,
    t('Are you sure you want to remove the field %field?', array(
      '%field' => $field['widget']['label'])),
    'admin/build/node-type/'. $type['url_str'] .'/fields',
    t('If you have any content left in this field, it will be lost. This action cannot be undone.'),
    t('Remove'), t('Cancel'),
    'confirm'
  );

  return $output;
}

/**
 * Remove a field from a content type.
 */
function _content_admin_field_remove_submit($form, &$form_state) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];

  $type = content_types($form_values['type_name']);
  $field = $type['fields'][$form_values['field_name']];
  if ($type && $field && $form_values['confirm']) {
    if (field_delete_instance($form_values['field_name'], $form_values['type_name'])) {
      drupal_set_message(t('Removed field %field from %type.', array(
        '%field' => $field['widget']['label'],
        '%type' => $type['name'])));
    }
    else {
      drupal_set_message(t('There was a problem deleting %field from %type.', array(
        '%field' => $field['widget']['label'],
        '%type' => $type['name'])));
    }
    $form_state['redirect'] = 'admin/build/node-type/'. $type['url_str'] .'/fields';
  }
}

/**
 * Menu callback; presents the field editing page.
 */
function _content_admin_field(&$form_state, $type_name, $field_name) {
  $output = '';
  $type = content_types($type_name);
  $field = $type['fields'][$field_name];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$field['widget']['type']];

  drupal_set_title(isset($field['widget']['label']) ? $field['widget']['label'] : $field['field_name']);

  $form = array();
  $form['widget'] = array(
    '#type' => 'fieldset',
    '#title' => t('%type settings', array('%type' => $type['name'])),
    '#description' => t('These settings apply only to the %field field as it appears in the %type content type.', array(
      '%field' => $field['widget']['label'],
      '%type' => $type['name'])),
  );
  $options = array();
  foreach ($widget_types as $possible_widget_name => $possible_widget_type) {
    if (in_array($field['type'], $possible_widget_type['field types'])) {
      $options[$possible_widget_name] = $possible_widget_type['label'];
    }
  }
  if (count($options) == 1) {
    $key = array_keys($options);
    $default_widget = array_pop($key);
  }
  $form['widget']['widget_type'] = array(
    '#type' => 'radios',
    '#title' => t('Widget'),
    '#options' => $options,
    '#default_value' => $field['widget']['type'] ? $field['widget']['type'] : $default_widget,
    '#required' => TRUE,
  );
  $form['widget']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $field['widget']['label'],
    '#required' => TRUE,
  );
  $form['widget']['weight'] = array(
    '#type' => 'hidden',
    '#default_value' => $field['widget']['weight'],
  );
  $additions = module_invoke($widget_type['module'], 'widget_settings', 'form', $field['widget']);
  if (is_array($additions)) {
    $form['widget'] = array_merge($form['widget'], $additions);
  }
  $form['widget']['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Help text'),
    '#default_value' => $field['widget']['description'],
    '#rows' => 5,
    '#description' => t('Instructions to present to the user below this field on the editing form.'),
    '#required' => FALSE,
  );

  // Add handling for default value if not provided by field.
  if (content_callback('widget', 'default value', $field) == CONTENT_CALLBACK_DEFAULT) {
    $form['widget']['default_value_fieldset'] = array(
      '#type' => 'fieldset',
      '#title' => t('Default value'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    // Default value widget.
    $default_value = isset($field['widget']['default_value']) ? $field['widget']['default_value'] : array();
    $widget_form = array('#node' => (object) array('type' => $type_name));
    $widget_form_state = array('values' => array($field['field_name'] => $default_value));
    // Make sure the default value is not a required field.
    $widget_field = $field;
    $widget_field['required'] = FALSE;
    module_load_include('inc', 'content', 'includes/content.node_form');
    $form_element = content_field_form($widget_form, $widget_form_state, $widget_field, 0);
    $form['widget']['default_value_fieldset']['default_value_widget'] = $form_element;
    $form['widget']['default_value_fieldset']['default_value_widget']['#tree'] = TRUE;
    // Set up form info that the default value widget will need to find in the form.
    $form['#field_info'] = array($widget_field['field_name'] => $widget_field);

    // Advanced : PHP code.
    $form['widget']['default_value_fieldset']['advanced_options'] = array(
      '#type' => 'fieldset',
      '#title' => t('PHP code'),
      '#collapsible' => TRUE,
      '#collapsed' => empty($field['widget']['default_value_php']),
    );
    $db_info = content_database_info($field);
    $columns = array_keys($db_info['columns']);
    foreach ($columns as $key => $column) {
      $columns[$key] = "'$column' => value for $column";
    }
    $sample = 'array(
  0 => array('. implode(', ', $columns) .'),
  // You\'ll usually want to stop here. Provide more values
  // if you want your \'default value\' to be multi-valued :
  1 => array('. implode(', ', $columns) .'),
  2 => ...
);';
    $form['widget']['default_value_fieldset']['advanced_options']['default_value_php'] = array(
      '#type' => 'textarea',
      '#title' => t('Code'),
      '#default_value' => isset($field['widget']['default_value_php']) ? $field['widget']['default_value_php'] : '',
      '#rows' => 6,
      '#tree' => TRUE,
      '#description' => t("Advanced usage only: PHP code that returns a default value. Should not include &lt;?php ?&gt; delimiters. If this field is filled out, the value returned by this code will override any value specified above. Expected format :<pre>!sample</pre>Using !link_devel's 'devel load' tab on a %type content page might help you figure out the expected format.", array(
        '!sample' => $sample,
        '!link_devel' => l('devel.module', 'http://www.drupal.org/project/devel'),
        '%type' => $type_name)),
    );
  }

  $form['field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Global settings'),
    '#description' => t('These settings apply to the %field field in every content type in which it appears.', array('%field' => $field['widget']['label'])),
  );
  $form['field']['required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Required'),
    '#default_value' => $field['required'],
  );
  $form['field']['multiple'] = array(
    '#type' => 'select',
    '#title' => t('Number of values'),
    '#options' => array(1 => t('Unlimited'), 0 => 1) + drupal_map_assoc(range(2, 10)),
    '#default_value' => $field['multiple'],
    '#description' => t("Select a specific number of values for this field, or 'Unlimited' to provide an 'Add more' button so the users can add as many values as they like.") .'<br/><strong>'. t('Warning! Changing this setting after data has been created could result in the loss of data!') .'</strong>',
  );

  $form['field']['previous_field'] = array(
    '#type' => 'hidden',
    '#value' => serialize($field),
    );

  $additions = module_invoke($field_type['module'], 'field_settings', 'form', $field);
  if (is_array($additions)) {
    $form['field'] = array_merge($form['field'], $additions);
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save field settings'),
  );
  $form['type_name'] = array(
    '#type' => 'value',
    '#value' => $type_name,
  );
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field_name,
  );
  $form['type'] = array(
    '#type' => 'value',
    '#value' => $field['type'],
  );
  $form['module'] = array(
    '#type' => 'value',
    '#value' => $field['module'],
  );
  $form['widget_module'] = array(
    '#type' => 'value',
    '#value' => $field['widget']['module'],
  );
  $form['columns'] = array(
    '#type' => 'value',
    '#value' => $field['columns'],
  );
  return $form;
}

/**
 * Validate a field's settings.
 */
function _content_admin_field_validate($form, &$form_state) {
  include_once('./'. drupal_get_path('module', 'content') .'/includes/content.crud.inc');
  $form_values = $form_state['values'];
  $previous_field = unserialize($form_values['previous_field']);
  $field = content_field_instance_expand($form_values);
  $field['db_storage'] = content_storage_type($field);

  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$field['widget']['type']];

  if ($dropped_data = content_alter_db_analyze($previous_field, $field)) {
    // @TODO
    // This is a change that might result in loss of data.
    // Add a confirmation form here.
    // dsm($dropped_data);
  }

  module_invoke($widget_type['module'], 'widget_settings', 'validate', array_merge($field, $form_values));
  module_invoke($field_type['module'], 'field_settings', 'validate', array_merge($field, $form_values));

  // If content.module is handling the default value,
  // validate the result using the field validation.
  if (content_callback('widget', 'default value', $field) == CONTENT_CALLBACK_DEFAULT) {
    if (isset($form_values['default_value_php']) &&
    ($php = trim($form_values['default_value_php']))) {
      ob_start();
      $return = eval($php);
      ob_end_clean();
      if (!is_array($return)) {
        $error = TRUE;
      }
      else {
        foreach ($return as $item) {
          if (!is_array($item)) {
            $error = TRUE;
            break;
          }
        }
      }
      if ($error) {
        $db_info = content_database_info($field);
        $columns = array_keys($db_info['columns']);
        foreach ($columns as $key => $column) {
          $columns[$key] = "'$column' => value for $column";
        }
        $sample = 'array(
  0 => array('. implode(', ', $columns) .'),
  // You\'ll usually want to stop here. Provide more values
  // if you want your \'default value\' to be multi-valued :
  1 => array('. implode(', ', $columns) .'),
  2 => ...
);';
        form_set_error('default_value_php', t('The default value PHP code returned an incorrect value.<br/>Expected format: <pre>!sample</pre> Returned value: @value', array(
          '!sample' => $sample,
          '@value' => print_r($return, true))));
        return;
      }
      else {
        $default_value = $return;
        $is_code = TRUE;
        form_set_value(array('#parents' => array('default_value_php')), $php, $form_state);
        form_set_value(array('#parents' => array('default_value')), array(), $form_state);
      }
    }
    elseif (!empty($form_values['default_value_widget'])) {
      // Fields that handle their own multiple values may use an expected
      // value as the top-level key, so just pop off the top element.
      $key = array_shift(array_keys($form_values['default_value_widget']));
      $default_value = $form_values['default_value_widget'][$key];
      $is_code = FALSE;
      form_set_value(array('#parents' => array('default_value_php')), '', $form_state);
      form_set_value(array('#parents' => array('default_value')), $default_value, $form_state);
    }
    if (isset($default_value)) {
      $node = array();
      $node[$form_values['field_name']] = $default_value;
      $field['required'] = FALSE;
      $field_function = $field_type['module'] .'_field';

      // Widget now does its own validation, should be no need
      // to add anything for widget validation here.

      if (function_exists($field_function)) {
        $field_function('validate', $node, $field, $default_value, NULL, NULL);
      }
      // The field validation routine won't set an error on the right field,
      // so set it here.
      if (form_get_errors()) {
        if (trim($form_values['default_value_php'])) {
          form_set_error('default_value_php', t('The default value PHP code created @value which is invalid.', array(
            '@value' => print_r($default_value, true))));
        }
        else {
          form_set_error('default_value', t('The default value is invalid.'));
        }
      }
    }
  }
}

/**
 * Save a field's settings after editing.
 */
function _content_admin_field_submit($form, &$form_state) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];

  // If the widget type has changed, update the widget module, too.
  $widget_types = _content_widget_types();
  $form_values['widget_module'] = $widget_types[$form_values['widget_type']]['module'];
  unset($form_values['default_value_widget']);

  $field_instance = (object)$form_values;
  field_update_instance($field_instance);

  drupal_set_message(t('Saved field %label.', array('%label' => $form_values['label'])));
  $type   = content_types($form_values['type_name']);
  $form_state['redirect'] = 'admin/build/node-type/'. $type['url_str'] .'/fields';
}
