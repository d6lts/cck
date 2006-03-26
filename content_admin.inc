<?php
// $Id$

/**
 * @file
 * Administrative interface for content type creation.
 */


/**
 * Menu callback; presents a listing of content types.
 */
function _content_admin_type_overview() {
  $types = _content_types();

  $header = array(t('Label'), t('Name'), t('Description'), array('data' => t('Operations'), 'colspan' => 3));
  $rows = array();
  foreach ($types as $type) {
    $row = array();
    $row[] = l(t($type->label), 'admin/node/types/'. $type->type_name .'/fields');
    $row[] = $type->type_name;
    $row[] = $type->description;

    $row[] = l(t('edit'), 'admin/node/types/'. $type->type_name);
    $row[] = l(t('duplicate'), 'admin/node/types/'. $type->type_name .'/duplicate');
    $row[] = l(t('delete'), 'admin/node/types/'. $type->type_name .'/delete');

    $rows[] = $row;
  }

  $output = theme('table', $header, $rows);

  return $output;
}

/**
 * Menu callback; lists all defined fields for quick reference.
 */
function _content_admin_type_fields() {
  $fields = _content_fields();

  $header = array(t('Name'), t('Type'), t('Content types'));
  $rows = array();
  foreach ($fields as $field) {
    $row = array();
    $row[] = $field['field_name'];
    $row[] = $field['type'];

    $types = array();
    $result = db_query("SELECT nt.label FROM {node_field_instance} nfi LEFT JOIN {node_type} nt ON nt.type_name = nfi.type_name WHERE nfi.field_name = '%s' ORDER BY nt.label ASC", $field['field_name']);
    while ($type = db_fetch_object($result)) {
      $types[] = $type->label;
    }
    $row[] = implode (', ', $types);

    $rows[] = $row;
  }

  $output = theme('table', $header, $rows);

  return $output;
}

/**
 * Menu callback; handles the editing of a content type.
 */
function _content_admin_type_edit($type_name = '', $original_type_name = '') {
  $types = _content_types();
  if (isset($types[$type_name])) {
    $type = $types[$type_name];
  }
  else {
    if (isset($types[$original_type_name])) {
      $type = $types[$original_type_name];
    }
    else {
      $type = new stdClass();
      $type->label = '';
      $type->description = '';
      $type->help = '';
      $type->title_label = 'Title';
    }
  }

  $form = array();
  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => $type->label,
    '#description' => t('The human-readable name of this content type.'),
    '#required' => TRUE,
  );
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#default_value' => $type->description,
    '#rows' => 10,
    '#description' => t('A brief description of the content type.'),
    '#required' => FALSE,
  );
  $form['help'] = array(
    '#title' => t('Help text'),
    '#type' => 'textarea',
    '#default_value' => $type->help,
    '#rows' => 10,
    '#description' => t('Instructions to present to the user when adding new content of this type.'),
    '#required' => FALSE,
  );
  $form['title_label'] = array(
    '#title' => t('Title field label'),
    '#type' => 'textfield',
    '#default_value' => $type->title_label,
    '#description' => t('The label for the title field.'),
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save content type'),
  );
  $form['type_name'] = array(
    '#type' => 'value',
    '#value' => $type_name,
  );
  $form['original_type_name'] = array(
    '#type' => 'value',
    '#value' => $original_type_name,
  );
  return drupal_get_form('_content_admin_type_edit', $form);
}

/**
 * Save a content type after editing.
 */
function _content_admin_type_edit_submit($form_id, $form_values) {
  $types = _content_types();
  if (!isset($types[$form_values['type_name']])) {
    // Find a valid, computer-friendly type name.
    $form_values['type_name'] = trim($form_values['label']);
    $form_values['type_name'] = drupal_strtolower($form_values['type_name']);
    $form_values['type_name'] = str_replace(array(' ', '-'), '_', $form_values['type_name']);
    $form_values['type_name'] = preg_replace('/[^a-z0-9_]/', '', $form_values['type_name']);
    $form_values['type_name'] = 'content-'. $form_values['type_name'];
    $form_values['type_name'] = substr($form_values['type_name'], 0, 32);
    if (isset($types[$form_values['type_name']])) {
      $counter = 0;
      do {
        $new_name = substr($form_values['type_name'], 0, 30) .'_'. $counter++;
      } while (isset($types[$new_name]));
      $form_values['type_name'] = $new_name;
    }

    db_query("INSERT INTO {node_type} (type_name, label, description, help, title_label) VALUES ('%s', '%s', '%s', '%s', '%s')", $form_values['type_name'], $form_values['label'], $form_values['description'], $form_values['help'], $form_values['title_label']);
  }
  else {
    db_query("UPDATE {node_type} SET label = '%s', description = '%s', help = '%s', title_label = '%s' WHERE type_name = '%s'", $form_values['label'], $form_values['description'], $form_values['help'], $form_values['title_label'], $form_values['type_name']);
  }

  if (isset($form_values['original_type_name'])) {
    // Duplicate the field structure from the old content type.
    db_query("INSERT INTO {node_field_instance} (field_name, type_name, weight, label, widget_type, widget_settings) SELECT field_name, '%s', weight, label, widget_type, widget_settings FROM {node_field_instance} WHERE type_name = '%s'", $form_values['type_name'], $form_values['original_type_name']);
  }

  drupal_set_message(t('Saved content type %type.', array('%type' => theme('placeholder', $form_values['label']))));

  content_clear_type_cache();
  menu_rebuild();

  drupal_goto('admin/node/types/'. $form_values['type_name']);
}

/**
 * Menu callback; delete a content type.
 */
function _content_admin_type_delete($type_name = '') {
  $types = _content_types();
  $type = $types[$type_name];

  $form = array();
  $form['type_name'] = array('#type' => 'value', '#value' => $type_name);
  return confirm_form('_content_admin_type_delete', $form, t('Are you sure you want to delete the content type %type?', array('%type' => theme('placeholder', $type->label))), 'admin/node/types', t('If you have any content left in this content type, it will be permanently deleted. This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Delete a content type.
 */
function _content_admin_type_delete_submit($form_id, $form_values) {
  $types = _content_types();
  $type = $types[$form_values['type_name']];

  if ($type && $form_values['confirm']) {
    // Delete all nodes of this content type.
    $result = db_query("SELECT nid FROM {node} WHERE type = '%s'", $form_values['type_name']);
    while ($node = db_fetch_object($result)) {
      node_delete($node->nid);
    }
    db_query("DELETE FROM {node_field_instance} WHERE type_name = '%s'", $form_values['type_name']);
    db_query("DELETE FROM {node_type} WHERE type_name = '%s'", $form_values['type_name']);

    drupal_set_message(t('Deleted content type %type.', array('%type' => theme('placeholder', $type->label))));
    content_clear_type_cache();
    drupal_goto('admin/node/types');
  }
}


/**
 * Menu callback; presents a listing of fields for a content type.
 */
function _content_admin_field_overview($type_name) {
  $types = _content_types();
  $type = $types[$type_name];
  $field_types = _content_field_types();

  $header = array(t('Label'), t('Name'), t('Type'), array('data' => t('Operations'), 'colspan' => 2));
  $rows = array();
  $rows[] = array(t($type->title_label), 'title', '', '', '');

  $c = db_query(db_rewrite_sql("SELECT v.*, n.type FROM {vocabulary} v INNER JOIN {vocabulary_node_types} n ON v.vid = n.vid WHERE n.type = '%s' ORDER BY v.weight, v.name", 'v', 'vid'), $type_name);
  while ($vocabulary = db_fetch_object($c)) {
    $rows[] = array(check_plain($vocabulary->name), 'taxonomy', '', '', '');
  }

  foreach ($type->fields as $field) {
    $row = array();
    $row[] = $field['widget']['label'];
    $row[] = $field['field_name'];
    $row[] = $field_types[$field['type']]['label'];

    $row[] = l(t('configure'), 'admin/node/types/'. $type_name .'/fields/'. $field['field_name']);
    $row[] = l(t('remove'), 'admin/node/types/'. $type_name .'/fields/'. $field['field_name'] .'/remove');

    $rows[] = $row;
  }

  $output = theme('table', $header, $rows);

  return $output;
}

/**
 * Menu callback; presents the form for adding a new field.
 */
function _content_admin_field_add($type_name) {
  $types = _content_types();
  $type = $types[$type_name];
  $field_types = _content_field_types();
  $fields = _content_fields();
  $widget_types = _content_widget_types();
  $output = '';

  $options = array();
  foreach ($fields as $field) {
    if (!isset($type->fields[$field['field_name']]))
    $options[$field['field_name']] = t($field['widget']['label']) .' ('. $field['field_name'] .')';
  }
  if ($options) {
    $form = array();
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
    $output .= drupal_get_form('_content_admin_field_add_existing', $form);
  }

  $field_type_options = array();
  foreach ($field_types as $field_name => $field_type) {
    foreach ($widget_types as $widget_name => $widget_type) {
      if (in_array($field_name, $widget_type['field types'])) {
        $field_type_options[$field_name .'-'. $widget_name] = $widget_type['label'];
      }
    }
  }
  if (count($field_type_options) > 0) {
    $form = array();
    $form['new'] = array(
      '#type' => 'fieldset',
      '#title' => t('Create new field'),
    );
    $form['new']['widget']['label'] = array(
      '#title' => t('Label'),
      '#type' => 'textfield',
      '#default_value' => '',
      '#description' => t('The human-readable name of this field.'),
      '#required' => TRUE,
    );
    $form['new']['field_widget_type'] = array(
      '#type' => 'radios',
      '#title' => t('Field type'),
      '#required' => TRUE,
      '#options' => $field_type_options,
      '#theme' => 'content_admin_field_add_new_field_widget_type',
    );

    $form['new']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Create field'),
    );
    $form['new']['type_name'] = array(
      '#type' => 'value',
      '#value' => $type_name,
    );
    $output .= drupal_get_form('_content_admin_field_add_new', $form);
  }
  else {
    drupal_set_message(t('No field modules are enabled. You need to <a href="%modules_url">enable one</a>, such as text.module, before you can add new fields.', array('%modules_url' => url('admin/modules'))), 'error');
  }

  return $output;
}

function theme_content_admin_field_add_new_field_widget_type($form) {
  $types = _content_types();
  $type = $types[$type_name];
  $field_types = _content_field_types();
  $fields = _content_fields();
  $widget_types = _content_widget_types();
  $output = '';

  $output .= '<dl>';
  foreach ($field_types as $field_name => $field_type) {
    $output .= '<dt>'. $field_type['label'] .'</dt>';
    foreach ($widget_types as $widget_name => $widget_type) {
      if (in_array($field_name, $widget_type['field types'])) {
        $output .= '<dd>'. form_render($form[$field_name .'-'. $widget_name]) .'</dd>';
      }
    }
  }
  $output .= '</dl>';

  return $output;
}

/**
 * Add an existing field to a content type.
 */
function _content_admin_field_add_existing_submit($form_id, $form_values) {
  $prior_instance = db_fetch_object(db_query("SELECT weight, label, widget_type, widget_settings FROM {node_field_instance} WHERE field_name = '%s'", $form_values['field_name']));
  if (!$prior_instance) {
    $prior_instance = new stdClass();
    $prior_instance->weight = 0;
    $prior_instance->label = $form_values['field_name'];
    $prior_instance->widget_type = '';
    $prior_instance->widget_settings = '';
  }
  db_query("INSERT INTO {node_field_instance} (field_name, type_name, weight, label, widget_type, widget_settings) VALUES ('%s', '%s', %d, '%s', '%s', '%s')", $form_values['field_name'], $form_values['type_name'], $prior_instance->weight, $prior_instance->label, $prior_instance->widget_type, $prior_instance->widget_settings);

  drupal_set_message(t('Added field %label.', array('%label' => theme('placeholder', $prior_instance->label))));
  content_clear_type_cache();
  drupal_goto('admin/node/types/'. $form_values['type_name'] .'/fields');
}

/**
 * Create a new field for a content type.
 */
function _content_admin_field_add_new_submit($form_id, $form_values) {
  // Find a valid, computer-friendly field name.

  $fields = _content_fields();

  $field_name = trim($form_values['label']);
  $field_name = drupal_strtolower($field_name);
  $field_name = str_replace(array(' ', '-'), '_', $field_name);
  $field_name = preg_replace('/[^a-z0-9_]/', '', $field_name);
  $field_name = 'field_'. $field_name;
  $field_name = substr($field_name, 0, 31);
  if (isset($fields[$field_name])) {
    $counter = 0;
    do {
      $new_name = substr($field_name, 0, 29) .'_'. $counter++;
    } while (isset($fields[$new_name]));
    $field_name = $new_name;
  }

  $field_widget_type = explode('-', $form_values['field_widget_type']);
  db_query("INSERT INTO {node_field} (field_name, type) VALUES ('%s', '%s')", $field_name, $field_widget_type[0]);

  db_query("INSERT INTO {node_field_instance} (type_name, field_name, label, widget_type) VALUES ('%s', '%s', '%s', '%s')", $form_values['type_name'], $field_name, $form_values['label'], $field_widget_type[1]);

  drupal_set_message(t('Created field %label.', array('%label' => theme('placeholder', $form_values['label']))));
  content_clear_type_cache();
  drupal_goto('admin/node/types/'. $form_values['type_name'] .'/fields/'. $field_name);
}

/**
 * Menu callback; present a form for removing a field from a content type.
 */
function _content_admin_field_remove($type_name, $field_name) {
  $types = _content_types();
  $type = $types[$type_name];
  $field = $type->fields[$field_name];

  $form = array();
  $form['type_name'] = array(
    '#type' => 'value',
    '#value' => $type_name,
  );
  $form['field_name'] = array(
    '#type' => 'value',
    '#value' => $field_name,
  );
  return confirm_form('_content_admin_field_remove', $form, t('Are you sure you want to remove the field %field?', array('%field' => theme('placeholder', $field['widget']['label']))), 'admin/node/types/'. $type_name .'/fields', t('If you have any content left in this field, it will be lost. This action cannot be undone.'), t('Remove'), t('Cancel'));
}

/**
 * Remove a field from a content type.
 */
function _content_admin_field_remove_submit($form_id, $form_values) {
  $types = _content_types();
  $type = $types[$form_values['type_name']];
  $field = $type->fields[$form_values['field_name']];

  if ($type && $field && $form_values['confirm']) {
    db_query("DELETE FROM {node_field_instance} WHERE type_name = '%s' AND field_name = '%s'", $form_values['type_name'], $form_values['field_name']);

    drupal_set_message(t('Removed field %field from %type.', array('%field' => theme('placeholder', $field['widget']['label']), '%type' => theme('placeholder', $type->label))));
    if (!db_result(db_query("SELECT type_name FROM {node_field_instance} WHERE field_name = '%s'", $form_values['field_name']))) {
      db_query("DELETE FROM {node_field} WHERE field_name = '%s'", $form_values['field_name']);

      drupal_set_message(t('The field %field no longer exists in any content type, so it was deleted.', array('%field' => theme('placeholder', $field['widget']['label']))));
    }
    content_clear_type_cache();
    drupal_goto('admin/node/types/'. $form_values['type_name'] .'/fields');
  }
}


/**
 * Menu callback; presents the field editing page.
 */
function _content_admin_field($type_name, $field_name) {
  $output = '';
  $types = _content_types();
  $type = $types[$type_name];
  $field = $type->fields[$field_name];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$field['widget']['type']];

  $form = array();
  $form['widget'] = array(
    '#type' => 'fieldset',
    '#title' => t('Widget settings'),
    '#description' => t('These settings apply only to the %field field as it appears in the %type content type.', array('%field' => theme('placeholder', $field['widget']['label']), '%type' => theme('placeholder', $type->label))),
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
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#default_value' => $field['widget']['weight'],
    '#description' => t('In the node editing form, the heavier fields will sink and the lighter fields will be positioned nearer the top.'),
  );
  if ($additions = module_invoke($widget_type['module'], 'widget_settings', 'form', $field['widget'])) {
    $form['widget'] = array_merge($form['widget'], $additions);
  }

  $form['field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Field settings'),
    '#description' => t('These settings apply to the %field field in every content type in which it appears.', array('%field' => theme('placeholder', $field['widget']['label']))),
  );
  $form['field']['required'] = array(
    '#type' => 'checkbox',
    '#title' => t('Required'),
    '#default_value' => $field['required'],
  );
  $form['field']['multiple'] = array(
    '#type' => 'checkbox',
    '#title' => t('Multiple values'),
    '#default_value' => $field['multiple'],
  );

  $form['field'] = array_merge($form['field'], module_invoke($field_type['module'], 'field_settings', 'form', $field));

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

  $output .= drupal_get_form('_content_admin_field', $form);

  return $output;
}

/**
 * Validate a field's settings.
 */
function _content_admin_field_validate($form_id, $form_values) {
  $types = _content_types();
  $type = $types[$form_values['type_name']];
  $field = $type->fields[$form_values['field_name']];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$field['widget']['type']];

  module_invoke($widget_type['module'], 'widget_settings', 'validate', array_merge($field, $form_values));
  module_invoke($field_type['module'], 'field_settings', 'validate', array_merge($field, $form_values));
}

/**
 * Save a field's settings after editing.
 */
function _content_admin_field_submit($form_id, $form_values) {
  $types = _content_types();
  $type = $types[$form_values['type_name']];
  $field = $type->fields[$form_values['field_name']];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$form_values['widget_type']];

  $widget_settings = array();
  if (is_array($setting_names = module_invoke($widget_type['module'], 'widget_settings', 'save', $field))) {
    foreach ($setting_names as $setting) {
      $widget_settings[$setting] = $form_values[$setting];
    }
  }
  $field_settings = array();
  if (is_array($setting_names = module_invoke($field_type['module'], 'field_settings', 'save', $field))) {
    foreach ($setting_names as $setting) {
      $field_settings[$setting] = $form_values[$setting];
    }
  }

  db_query("UPDATE {node_field_instance} SET weight = %d, label = '%s', widget_type = '%s', widget_settings = '%s' WHERE type_name = '%s' AND field_name = '%s'", $form_values['weight'], $form_values['label'], $form_values['widget_type'], serialize($widget_settings), $form_values['type_name'], $form_values['field_name']);

  db_query("UPDATE {node_field} SET required = %d, multiple = %d, global_settings = '%s' WHERE field_name = '%s'", $form_values['required'], $form_values['multiple'], serialize($field_settings), $form_values['field_name']);

  drupal_set_message(t('Saved field %field.', array('%field' => theme('placeholder', $form_values['label']))));
  content_clear_type_cache();
  drupal_goto('admin/node/types/'. $form_values['type_name'] .'/fields');
}
