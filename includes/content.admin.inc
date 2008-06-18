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
      $types[] = l($type['name'], 'admin/content/node-type/'. $content_type['url_str'] .'/fields');
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
  $fields = $type['fields'];
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
      'configure' => array('#value' => l(t('Configure'), 'admin/content/node-type/'. $type['url_str'] .'/fields/'. $field['field_name'] .'/edit')),
      'remove' => array('#value' => l(t('Remove'), 'admin/content/node-type/'. $type['url_str'] .'/fields/'. $field['field_name'] .'/remove')),
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
      'configure' => array('#value' => l(t('Configure'), 'admin/content/node-type/'. $type['url_str'] .'/groups/'. $group['group_name'] .'/edit')),
      'remove' => array('#value' => l(t('Remove'), 'admin/content/node-type/'. $type['url_str'] .'/groups/'. $group['group_name'] .'/remove')),
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
      $field = content_field_instance_collapse($field);
      content_field_instance_update($field);
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
  $output = '';
  $options = content_field_type_options();
  if (!empty($options)) {
    $output .= drupal_get_form('content_admin_field_main', array('type_name' => $type_name));
  }
  else {
    drupal_set_message(t('No field modules are enabled. You need to <a href="!modules_url">enable one</a>, such as text.module, before you can add new fields.', array('!modules_url' => url('admin/build/modules'))), 'error');
  }
  return $output;
}

/**
 * Arrays of field type and widget type options.
 */
function content_field_type_options($type = NULL) {
  $field_types = _content_field_types();
  $widget_types = _content_widget_types();
  $field_type_options = array();
  $widget_type_options = array();
  foreach ($field_types as $field_type_name => $field_type) {
    foreach ($widget_types as $widget_type_name => $widget_type) {
      if (in_array($field_type_name, $widget_type['field types'])) {
        $field_type_options[$field_type_name] = $field_type['label'];
        $widget_type_options[$field_type_name][$widget_type_name] = $widget_type['label'];
      }
    }
  }
  asort($field_type_options);
  if (!empty($type)) {
    return $widget_type_options[$type];
  }
  return $field_type_options;
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
    $form['existing']['existing_field_name'] = array(
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

/**
 * Add an existing field to a content type.
 */
function _content_admin_field_add_existing_submit($form, &$form_state) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];
  $form_values['field_name'] = $form_values['existing_field_name'];

  if (content_field_instance_create($form_values)) {
    drupal_set_message(t('Added field %label.', array(
      '%label' => $form_values['field_name'])));  }
  else {
    drupal_set_message(t('There was a problem adding field %label.', array(
      '%label' => $form_values['field_name'])));
  }
  $type = content_types($form_values['type_name']);
  $form_state['redirect'] = 'admin/content/node-type/'. $type['url_str'] .'/fields';
}

/**
 * A form element for selecting field, widget, and label.
 */
function content_admin_field_main(&$form_state, $form_values) {
  module_load_include('inc', 'content', 'includes/content.crud');
  $form = array();
  if (!empty($form_state['storage'])) {
    $form_values = $form_state['storage'];
  }

  $type_name = $form_values['type_name'];
  $type = content_types($type_name);
  $field_name = isset($form_values['field_name']) ? $form_values['field_name'] : '';
  $field_type = isset($form_values['type']) ? $form_values['type'] : '';
  $label = isset($form_values['label']) ? $form_values['label'] : '';

  // Remove the 'field_' prefix from the field name.
  $trimmed_field_name = $field_name;
  if (substr($field_name, 0, 6) == 'field_') {
    $trimmed_field_name = substr($field_name, 6);
  }

  // If we haven't chosen a field name yet, include an option to
  // share an existing field instead.
  if (empty($field_name)) {
    $form['#prefix'] = drupal_get_form('_content_admin_field_add_existing', $type_name);
  }

  // See if we're editing an existing field or a new field.
  $instances = content_field_instance_read(array('field_name' => $field_name, 'type_name' => $type_name));
  if (count($instances) < 1) {
    $form['new'] = array(
      '#type' => 'fieldset',
      '#title' => t('Create new field'),
    );
    $new = TRUE;
  }
  else {
    $form['new'] = array(
      '#type' => 'fieldset',
      '#title' => t('Edit basic information'),
    );
    $new = FALSE;
  }
  $form['new']['field_name'] = array(
    '#title' => t('Field name'),
    '#type' => 'textfield',
    '#default_value' => $trimmed_field_name,
    '#field_prefix' => 'field_',
    '#description' => t("The machine-readable name of the field."),
    '#required' => TRUE,
    );
  if (!empty($field_name)) {
    $form['new']['field_name']['#description'] .= t(" This name cannot be changed.");
    $form['new']['field_name']['#disabled'] = TRUE;
    $form['new']['field_name']['#required'] = FALSE;
    $form['new']['field_name']['#value'] = $field_name;
    $form['new']['field_name']['#field_prefix'] = '';
    $form['new_field_name'] = array('#type' => 'hidden', '#value' => FALSE);
  }
  else {
    $form['new']['field_name']['#description'] .= t(" This name cannot be changed later! The name will be prefixed with 'field_' and can include lowercase unaccented letters, numbers, and underscores. The length of the name, including the prefix, is limited to no more than 32 letters.");
    $form['new_field_name'] = array('#type' => 'hidden', '#value' => TRUE);
  }
  $form['new']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $label,
    '#required' => TRUE,
    '#description' => t('A human-readable name to be used as the label for this field in the %type content type.', array('%type' => $type['name'])),
  );

  if (empty($field_type)) {
    $form['new']['type'] = array(
      '#type' => 'select',
      '#title' => t('Field type'),
      '#required' => TRUE,
      '#options' => content_field_type_options(),
      '#default_value' => NULL,
      '#description' => t('The type of data you would like to store in the database with this field.'),
    );
    $form['widget_type'] = array('#type' => 'hidden', '#value' => '');
  }
  else {
    $form['new']['type'] = array(
      '#type' => 'select',
      '#title' => t('Field type'),
      '#options' => content_field_type_options(),
      '#default_value' => $field_type,
      '#description' => t('The type of data you would like to store in the database with this field. This option cannot be changed.'),
      '#disabled' => TRUE,
    );
    $form['new']['widget_type'] = array(
      '#type' => 'select',
      '#title' => t('Widget type'),
      '#required' => TRUE,
      '#options' => content_field_type_options($field_type),
      '#default_value' => $form_values['widget_type'],
      '#description' => t('The type of form element you would like to present to the user when creating this field in the %type content type.', array('%type' => $type['name'])),
    );
  }

  $form['type_name'] = array(
    '#type' => 'value',
    '#value' => $type_name,
  );
  $form['needs_update'] = array(
    '#type' => 'hidden',
    '#value' => !empty($form_values['needs_update']),
    );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );
  return $form;
}

/**
 *  Field name validation.
 */
function content_admin_field_main_validate($form, &$form_state) {
  $form_values = $form_state['values'];
  $field_name = $form_values['field_name'];

  // Add the 'field_' prefix.
  if (!empty($field_name) && substr($field_name, 0, 6) != 'field_') {
    $field_name = 'field_'. $field_name;
    form_set_value($form['new']['field_name'], $field_name, $form_state);
  }

  // Testing for a valid name only needs to be done the first time through.
  if (isset($form_values['new_field_name']) && $form_values['new_field_name']) {
    if (!preg_match('!^field_[a-z0-9_]+$!', $field_name)) {
      form_set_error('field_name', t('The field name %field_name is invalid. The name must include only lowercase unaccentuated letters, numbers, and underscores.', array(
        '%field_name' => $field_name)));
    }
    if (strlen($field_name) > 32) {
      form_set_error('field_name', t('The field name %field_name is too long. The name is limited to 32 characters, including the \'field_\' prefix.', array(
        '%field_name' => $field_name)));
    }
    if (array_key_exists($field_name, content_fields())) {
      form_set_error('field_name', t('The field name %field_name already exists.', array(
        '%field_name' => $field_name)));
    }
    elseif ($field_name == 'field_instance') {
      form_set_error('field_name', t("The name 'field_instance' is a reserved name."));
    }
  }
}

/**
 * Create a new field for a content type.
 */
function content_admin_field_main_submit($form, &$form_state) {
  $form_values = $form_state['values'];
  $form_state['storage'] = $form_values;
  if (empty($form_values['type']) || empty($form_values['widget_type'])) {
    $form_state['rebuild'] = TRUE;
  }
  else {
    module_load_include('inc', 'content', 'includes/content.crud');
    // Unset $form_state['storage'] so FAPI doesn't force
    // $form_state['rebuild'] to TRUE.
    unset($form_state['storage']);
    $form_state['rebuild'] = FALSE;

    $label = $form_values['label'];

    // Set the right module information
    $field_types = _content_field_types();
    $widget_types = _content_widget_types();
    $form_values['module'] = $field_types[$form_values['type']]['module'];
    $form_values['widget_module'] = $widget_types[$form_values['widget_type']]['module'];
    // Are we creating or updating this field?
    $instances = content_field_instance_read(array('field_name' => $form_values['field_name'], 'type_name' => $form_values['type_name']));
    if (sizeof($instances) < 1) {
      if (content_field_instance_create($form_values)) {
        drupal_set_message(t('Created field %label.', array(
          '%label' => $label)));
      }
      else {
        drupal_set_message(t('There was a problem creating field %label.', array(
          '%label' => $label)));
      }
    }
    else {
      // If we're updating, make sure we retain previous values and
      // only over-write changed values.
      $field = array_merge(content_field_instance_collapse($instances[0]), $form_values);
      if (content_field_instance_update($field)) {
        drupal_set_message(t('Update field %label.', array(
          '%label' => $label)));
      }
      else {
        drupal_set_message(t('There was a problem updating field %label.', array(
          '%label' => $label)));
      }
    }

    $type = content_types($form_values['type_name']);
    $form_state['redirect'] = 'admin/content/node-type/'. $type['url_str'] .'/fields/'. $form_values['field_name'];
  }
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
    'admin/content/node-type/'. $type['url_str'] .'/fields',
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
    if (content_field_instance_delete($form_values['field_name'], $form_values['type_name'])) {
      drupal_set_message(t('Removed field %field from %type.', array(
        '%field' => $field['widget']['label'],
        '%type' => $type['name'])));
    }
    else {
      drupal_set_message(t('There was a problem deleting %field from %type.', array(
        '%field' => $field['widget']['label'],
        '%type' => $type['name'])));
    }
    $form_state['redirect'] = 'admin/content/node-type/'. $type['url_str'] .'/fields';
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

  // See if we need to change the widget type or label.

  // TODO: $form_state['post']['op'] is deprecated, so this isn't the right
  // way to do this, need to find better method.
  if (isset($form_state['post']['op']) && $form_state['post']['op'] == t('Change basic information')) {
    module_load_include('inc', 'content', 'includes/content.crud');
    $form_values = content_field_instance_collapse($field);
    $form['markup'] = array('#value' => drupal_get_form('content_admin_field_main', $form_values));
    // Add a flag to use so we don't try to process the rest of this form.
    $form_state['change_basic'] = TRUE;
    return $form;
  }

  $form = array();
  $form['main'] = array(
    '#type' => 'fieldset',
    '#title' => t('%type basic information', array('%type' => $type['name'])),
    );
  $form['main']['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#value' => $field['widget']['label'],
    '#disabled' => TRUE,
  );
  $form['main']['field_name'] = array(
    '#type' => 'hidden',
    '#title' => t('Field name'),
    '#value' => $field['field_name'],
    '#disabled' => TRUE,
  );
  $form['main']['type'] = array(
    '#type' => 'hidden',
    '#title' => t('Field type'),
    '#options' => content_field_type_options(),
    '#value' => $field['type'],
    '#disabled' => TRUE,
  );
  $form['main']['widget_type'] = array(
    '#type' => 'select',
    '#title' => t('Widget type'),
    '#options' => content_field_type_options($field['type']),
    '#default_value' => $field['widget']['type'] ? $field['widget']['type'] : $default_widget,
    '#disabled' => TRUE,
  );
  $form['main']['change'] = array(
    '#type' => 'submit',
    '#value' => t('Change basic information'),
    '#validate' => array('content_admin_field_main_validate'),
    '#submit' => array('content_admin_field_main_submit'),
  );

  $form['widget'] = array(
    '#type' => 'fieldset',
    '#title' => t('%type settings', array('%type' => $type['name'])),
    '#description' => t('These settings apply only to the %field field as it appears in the %type content type.', array(
      '%field' => $field['widget']['label'],
      '%type' => $type['name'])),
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

    if (user_access('Use PHP input for field settings (dangerous - grant with care)')) {
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
    else {
      $form['widget']['default_value_fieldset']['advanced_options']['markup_default_value_php'] = array(
        '#type' => 'item',
        '#title' => t('Code'),
        '#value' => !empty($field['widget']['default_value_php']) ? '<code>'. check_plain($field['widget']['default_value_php']) .'</code>' : t('&lt;none&gt;'),
        '#description' => empty($field['widget']['default_value_php']) ? t("You're not allowed to input PHP code.") : t('This PHP code was set by an administrator and will override any value specified above.'),
      );
    }
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
  $form['widget']['label'] = array(
    '#type' => 'value',
    '#value' => $field['widget']['label'],
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
  if (isset($form_state['change_basic'])) {
    return;
  }
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
  if (isset($form_state['change_basic'])) {
    return;
  }
  module_load_include('inc', 'content', 'includes/content.crud');
  $form_values = $form_state['values'];
  content_field_instance_update($form_values);

  drupal_set_message(t('Saved field %label.', array('%label' => $form_values['label'])));
  $type   = content_types($form_values['type_name']);
  $form_state['redirect'] = 'admin/content/node-type/'. $type['url_str'] .'/fields';
}

/**
 * Content Schema Alter
 *
 * Alter the database schema.
 *
 * TODO figure out an API-safe way to use batching to update the nodes that
 * will be affected by this change so the node_save() hooks will fire.
 *
 */
function content_alter_schema($previous_field, $new_field) {
 content_alter_db($previous_field, $new_field);
}

/**
 * Schema Alter Analyze
 *
 * Analyze if changes will remove columns or delta values, thus losing data.
 * Do this so we can delete the data and fire the necessary hooks, before
 * we actually alter the schema.
 */
function content_alter_db_analyze($previous_field, $new_field) {
  $dropped = array();
  // There is no loss of data if there was no previous data.
  if (empty($previous_field)) {
    return $dropped;
  }

  // Analyze possible data loss from changes in storage type.
  if (!empty($previous_field) && !empty($new_field)) {
    // Changing from multiple to not multiple data, will cause loss of all
    // values greater than zero.
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD &&
    $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
      $dropped['delta'] = 0;
    }
    // Changing from one multiple value to another will cause loss of all
    // values for deltas greater than or equal to the new multiple value.
    elseif (isset($previous_field['multiple']) && isset($new_field['multiple'])) {
      if ($previous_field['multiple'] > $new_field['multiple'] &&
      $new_field['multiple'] > 1) {
        $dropped['delta'] = $new_field['multiple'];
      }
    }
  }

  // Analyze possible data loss from changes in field columns.
  $previous_schema = !empty($previous_field) ? content_table_schema($previous_field) : array('fields' => array());
  $new_schema = !empty($new_field) ? content_table_schema($new_field) : array('fields' => array());
  $dropped_columns = array_diff(array_keys($previous_schema['fields']), array_keys($new_schema['fields']));
  if ($dropped_columns) {
    $dropped['columns'] = $dropped_columns;
  }
//  if (empty($new_schema['fields'])) {
//    // No new columns, will lose all columns for a field.
//    foreach ($previous_schema['fields'] as $column => $attributes) {
//      $dropped['columns'][] = $column;
//    }
//  }
//  else {
//    // Check both old and new columns to see if we are deleting some columns for a field.
//    foreach ($previous_schema['fields'] as $column => $attributes) {
//      if (!isset($new_schema['fields'][$column])) {
//        $dropped['columns'][] = $column;
//      }
//    }
//  }

  return $dropped;
}

/**
 * Perform adds, alters, and drops as needed to synchronize the database with
 * new field definitions.
 */
function content_alter_db($previous_field, $new_field) {
  $ret = array();

  // One or the other of these must be valid.
  if (empty($previous_field) && empty($new_field)) {
    return $ret;
  }

  // Gather relevant information : schema, table name...
  $previous_schema = !empty($previous_field) ? content_table_schema($previous_field) : array();
  $new_schema = !empty($new_field) ? content_table_schema($new_field) : array();
  if (!empty($previous_field)) {
    $previous_db_info = content_database_info($previous_field);
    $previous_table = $previous_db_info['table'];
  }
  if (!empty($new_field)) {
    $new_db_info = content_database_info($new_field);
    $new_table = $new_db_info['table'];
  }
  // Deleting this field entirely is a simple case.
  if (empty($new_field)) {
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
      db_drop_table($ret, $previous_table);
    }
    else {
      foreach ($previous_schema['fields'] as $column => $attributes) {
        if (!in_array($column, array('nid', 'vid', 'delta'))) {
          db_drop_field($ret, $previous_table, $column);
        }
      }
    }
    return content_alter_db_cleanup($ret);
  }

  // All content types that have fields need a content type table.
  if (!empty($new_field)) {
    $base_tablename = _content_tablename($new_field['type_name'], CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
    if (!db_table_exists($base_tablename)) {
      db_create_table($ret, $base_tablename, content_table_schema());
    }
  }

  // Create new table and columns, if not already created.
  if (!db_table_exists($new_table)) {
    db_create_table($ret, $new_table, $new_schema);
  }
  else {
    // Or add fields to an existing table.
    foreach ($new_schema['fields'] as $column => $attributes) {
      if (!in_array($column, array('nid', 'vid', 'delta')) && !db_column_exists($new_table, $column)) {
        db_add_field($ret, $new_table, $column, $attributes);
      }
    }
  }

  // If this is a new field, we're done.
  if (empty($previous_field)) {
    return content_alter_db_cleanup($ret);
  }

  // If the previous table doesn't exist, we're done.
  // Could happen if someone tries to run a schema update from an
  // content.install update function more than once.
  if (!db_table_exists($previous_table)) {
    return content_alter_db_cleanup($ret);
  }

  // If changing data from one schema to another, see if changes require that
  // we drop multiple values or migrate data from one storage type to another.
  $migrate_columns = array_intersect_assoc($new_schema['fields'], $previous_schema['fields']);
  unset($migrate_columns['nid'], $migrate_columns['vid'], $migrate_columns['delta']);

  // If we're going from one multiple value a smaller one or to single,
  // drop all delta values higher than the new maximum delta value.
  // Not needed if the new multiple is unlimited or if the new table is the content table.
  if ($new_table != $base_tablename && $new_field['multiple'] < $previous_field['multiple'] && $new_field['multiple'] != 1) {
    db_query("DELETE FROM {". $new_table ."} WHERE delta >= ". max(1, $new_field['multiple']));
  }

  // If going from multiple to non-multiple, make sure the field tables have
  // the right database structure to accept migrated data.
  if ($new_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD && count($previous_schema['fields'])) {
      // Already using per-field storage; change multiplicity if needed.
      if ($previous_field['multiple'] > 0 && $new_field['multiple'] == 0) {
        db_drop_field($ret, $new_table, 'delta');
        db_drop_primary_key($ret, $new_table);
        db_add_primary_key($ret, $new_table, array('vid'));
      }
      else if ($previous_field['multiple'] == 0 && $new_field['multiple'] > 0) {
        db_add_field($ret, $new_table, 'delta', array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'default' => 0));
        db_drop_primary_key($ret, $new_table);
        db_add_primary_key($ret, $new_table, array('vid', 'delta'));
      }
    }
  }

  // Migrate data from per-content-type storage.
  if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE &&
  $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
    $columns = array_keys($migrate_columns);
    if ($new_field['multiple']) {
      db_query('INSERT INTO {'. $new_table .'} (vid, nid, delta, '. implode(', ', $columns) .') '.
        ' SELECT vid, nid, 0, '. implode(', ', $columns) .' FROM {'. $previous_table .'}');
    }
    else {
      db_query('INSERT INTO {'. $new_table .'} (vid, nid, '. implode(', ', $columns) .') '.
        ' SELECT vid, nid, '. implode(', ', $columns) .' FROM {'. $previous_table .'}');
    }
    foreach ($columns as $column_name) {
      db_drop_field($ret, $previous_table, $column_name);
    }
  }

  // Migrate data from per-field storage, and drop per-field table.
  if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD &&
  $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
    // In order to be able to use drupal_write_record, we need to
    // rebuild the schema now.
    content_alter_db_cleanup($ret);
    if ($previous_field['multiple']) {
      $result = db_query("SELECT * FROM {". $previous_table ."} c JOIN {node} n ON c.nid = n.nid WHERE delta = 0 AND n.type = '%s'", $new_field['type_name']);
    }
    else {
      $result = db_query("SELECT * FROM {". $previous_table ."} c JOIN {node} n ON c.nid = n.nid WHERE n.type = '%s'", $new_field['type_name']);
    }
    $record = array();
    while ($data = db_fetch_array($result)) {
      $record['nid'] = $data['nid'];
      $record['vid'] = $data['vid'];
      if ($previous_field['multiple']) {
        $record['delta'] = $data['delta'];
      }
      foreach ($migrate_columns as $column => $attributes) {
        $record[$column] = is_null($data[$column]) ? NULL : $data[$column];
      }
      if (db_result(db_query('SELECT COUNT(*) FROM {'. $new_table .
      '} WHERE vid = %d AND nid = %d', $data['vid'], $data['nid']))) {
        $keys = $new_field['multiple'] ? array('vid', 'delta') : array('vid');
        drupal_write_record($new_table, $record, $keys);
      }
      else {
        drupal_write_record($new_table, $record);
      }
    }
    db_drop_table($ret, $previous_table);
  }

  // Change modified columns that don't involve storage changes.
  foreach ($new_schema['fields'] as $column => $attributes) {
    if (isset($previous_schema['fields'][$column]) &&
    $previous_field['db_storage'] == $new_field['db_storage']) {
      if ($attributes != $previous_schema['fields'][$column]) {
        if (!in_array($column, array('nid', 'vid', 'delta'))) {
          db_change_field($ret, $new_table, $column, $column, $attributes);
        }
      }
    }
  }

  // Remove obsolete columns.
  foreach ($previous_schema['fields'] as $column => $attributes) {
    if (!isset($new_schema['fields'][$column])) {
      if (!in_array($column, array('nid', 'vid', 'delta'))) {
        db_drop_field($ret, $previous_table, $column);
      }
    }
  }

  // TODO : debugging stuff - should be removed
  if (module_exists('devel')) {
    //dsm($ret);
  }
  return $ret;
}

/**
 * Helper function for handling cleanup operations when schema changes are made.
 */
function content_alter_db_cleanup($ret) {
  // Rebuild the whole database schema.
  // TODO : this could be optimized. We don't need to rebuild in *every case*...
  // Or do we? This affects the schema and menu and may have unfortunate
  // delayed effects if we don't clear everything out at this point.
  content_clear_type_cache(TRUE);

  // TODO : debugging stuff - should be removed
  if (module_exists('devel')) {
    //dsm($ret);
  }
  return $ret;

}

/**
 * Batching process for changing the field schema,
 * running each affected node through node_save() first, to
 * fire all hooks.
 *
 * TODO This is just a placeholder for now because batching can't be safely
 * used with API hooks. Need to come back and figure out how to incorporate
 * this and get it working properly when the fields are altered via the API.
 */
function content_alter_fields($previous_field, $new_field) {
  // See what values need to be updated in the field data.
  $mask = content_alter_db_mask($previous_field, $new_field);

  // We use batch processing to prevent timeout when updating a large number
  // of nodes. If there is no previous data to adjust, we can just go straight
  // to altering the schema, otherwise use batch processing to update
  // the database one node at a time, then update the schema.
  if (empty($mask)) {
    return content_alter_db($previous_field, $new_field);
  }
  $updates = array(
    'mask' => $mask['mask'],
    'alt_mask' => $mask['alt_mask'],
    'delta' => $mask['delta'],
    );
  $batch = array(
    'operations' => array(
      array('content_field_batch_update', array($previous_field['field_name'] => $updates)),
      array('content_alter_db', array($previous_field, $new_field))
    ),
    'finished' => '_content_alter_fields_finished',
    'title' => t('Processing'),
    'error_message' => t('The update has encountered an error.'),
    'file' => './'. drupal_get_path('module', 'content') .'/includes/content.admin.inc',
  );
  batch_set($batch);
  if (!empty($url)) {
    batch_process($url, $url);
  }
}

/**
 * Content Replace Fields 'finished' callback.
 */
function _content_alter_fields_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(t('The database has been altered and data has been migrated or deleted.'));
  }
  else {
    drupal_set_message(t('An error occurred and database alteration did not complete.'), 'error');
    $message = format_plural(count($results), '1 item successfully processed:', '@count items successfully processed:');
    $message .= theme('item_list', $results);
    drupal_set_message($message);
  }
}

/**
 * Create a mask for the column data that should be deleted in each field.
 *
 * This is a bit tricky. We could theoretically have some columns
 * that should be set to empty and others with valid info that should
 * not be emptied out. But if delta values > X are to be wiped out, they
 * need to wipe out even columns that still have values. And the NULL
 * values in these columns after the alteration may be enough to make
 * the item 'empty', as defined by hook_content_is_empty(), even if
 * some columns still have values, so all these things need to be tested.
 */
function content_alter_db_mask($previous_field, $new_field) {
  // Get an array of column values that will be dropped by this
  // schema change and create a mask to feed to content_batch_update.

  $dropped = content_alter_db_analyze($previous_field, $new_field);
  if (empty($dropped)) {
    return array();
  }
  $mask = array('mask' => array());
  foreach (array_keys($previous_field['columns']) as $column_name) {
    // The basic mask will empty the dropped columns.
    if (isset($dropped['columns']) && in_array($column_name, $dropped['columns'])) {
      $mask['mask'][$column_name] = NULL;
    }
    // Over the delta we'll empty all columns.
    if (isset($dropped['delta'])) {
      $mask['alt_mask'][$column_name] = NULL;
    }
  }
  if (isset($dropped['delta'])) {
    $mask['delta'] = $dropped['delta'];
  }
  return $mask;
}

/**
 * Content Field Batch Update Operation
 *
 * Find all nodes that contain a field and update their values.
 *
 * @param $updates
 *   an array like:
 *   'field_name' => array(
 *     'mask' => array()
 *       // Keyed array of column names and replacement values for use
 *       // below delta, or for all values if no delta is supplied.
 *     'alt_mask' => array()
 *       // Optional, keyed array of column names and replacement values for use
 *       // at or above delta, if a delta is supplied.
 *     'delta' => #
 *       // Optional, the number to use as the delta value where you switch from
 *       // one mask to the other.
 *     ),
 */
function content_field_batch_update($updates, &$context) {
  if (empty($field)) {
    $context['finished'] = 1;
    return;
  }
  $field_name = $updates['field_name'];
  $field = content_fields($field_name);

  if (!isset($context['sandbox']['progress'])) {
    $db_info = content_database_info($field);

    // Might run into non-existent tables when cleaning up a corrupted
    // database, like some of the old content storage changes in the
    // .install files.
    if (!db_table_exists($db_info['table'])) {
      return $context['finished'] = 1;
    }
    $nodes = array();
    $result = db_query("SELECT nid FROM {". $db_info['table'] ."}");
    while ($node = db_fetch_array($result)) {
      $nodes[] = $node['nid'];
    }
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($nodes);
    $context['sandbox']['nodes'] = $nodes;
  }

  // Process nodes by groups of 5.
  $count = min(5, count($context['sandbox']['nodes']));

  for ($i = 1; $i <= $count; $i++) {
    // For each nid, load the node, empty the column values
    // or the whole field, and re-save it.
    $nid = array_shift($context['sandbox']['nodes']);
    $node = content_field_replace($nid, array($updates));

    // Store result for post-processing in the finished callback.
    $context['results'][] = l($node->title, 'node/'. $node->nid);

    // Update our progress information.
    $context['sandbox']['progress']++;
    $context['message'] = t('Processing %title', array('%title' => $node->title));
  }

  // Inform the batch engine that we are not finished,
  // and provide an estimation of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Content Field Replace
 *
 * Replace field values in a node from an array of update values.
 *
 * Supply an array of one or more fields and masks of field column values
 * to be replaced into field values, one mask for basic values and an optional
 * different mask for values in field items equal to or higher than a
 * specified delta.
 *
 * The masks should contain only the column values to be substituted in.
 * The supplied values will be merged into the existing values to replace
 * only the values in the mask, leaving all other values unchanged.
 *
 * The ability to set different masks starting at a delta allows the
 * possibility of setting values above a certain delta to NULL prior
 * to altering the database schema.
 *
 * @param $nid
 * @param $updates
 *   an array like:
 *   'field_name' => array(
 *     'mask' => array()
 *       // Keyed array of column names and replacement values for use
 *       // below delta, or for all values if no delta is supplied.
 *     'alt_mask' => array()
 *       // Optional, keyed array of column names and replacement values for use
 *       // at or above delta, if a delta is supplied.
 *     'delta' => #
 *       // Optional, the number to use as the delta value where you switch from
 *       // one mask to the other.
 *     ),
 */
function content_field_replace($nid, $updates) {
  $node = node_load($nid, NULL, TRUE);
  foreach ($updates as $field_name => $update) {
    $items = isset($node->$field_name) ? $node->$field_name : array();
    foreach ($items as $delta => $value) {
      $field_mask = (isset($update['delta']) && isset($update['alt_mask']) && $delta >= $update['delta']) ? $update['alt_mask'] : $mask['mask'];
      // Merge the mask into the field values to do the replacements.
      $items[$delta] = array_merge($items[$delta], $field_mask);
    }
    // Test if the new values will make items qualify as empty.
    $items = content_set_empty($field, $items);
    $node->$field_name = $items;
  }
  node_save($node);
  return $node;
}
