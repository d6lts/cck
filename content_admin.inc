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

  $header = array(t('Name'), t('Type'), t('Content types'));
  $rows = array();
  foreach ($fields as $field) {
    $row = array();
    $row[] = $field['field_name'];
    $row[] = $field['type'];

    $types = array();
    $result = db_query("SELECT nt.name FROM {node_field_instance} nfi LEFT JOIN {node_type} nt ON nt.type = nfi.type_name WHERE nfi.field_name = '%s' ORDER BY nt.name ASC", $field['field_name']);
    while ($type = db_fetch_array($result)) {
      $types[] = $type['name'];
    }
    $row[] = implode (', ', $types);

    $rows[] = $row;
  }

  $output = theme('table', $header, $rows);

  return $output;
}

/**
 * Menu callback; presents a listing of fields for a content type.
 */
function _content_admin_field_overview($type_name) {
  
  $type = content_types($type_name);
  $field_types = _content_field_types();
  
  $header = array(t('Label'), t('Name'), t('Type'), array('data' => t('Operations'), 'colspan' => 2));
  $rows = array();
  $rows[] = array(t($type['title_label']), 'title', '', '', '');

  $c = db_query(db_rewrite_sql("SELECT v.*, n.type FROM {vocabulary} v INNER JOIN {vocabulary_node_types} n ON v.vid = n.vid WHERE n.type = '%s' ORDER BY v.weight, v.name", 'v', 'vid'), $type_name);
  while ($vocabulary = db_fetch_object($c)) {
    $rows[] = array(check_plain($vocabulary->name), 'taxonomy', '', '', '');
  }

  foreach ($type['fields'] as $field) {
    $row = array();
    $row[] = $field['widget']['label'];
    $row[] = $field['field_name'];
    $row[] = $field_types[$field['type']]['label'];

    $row[] = l(t('configure'), 'admin/content/types/'. $type['url_str'] .'/fields/'. $field['field_name']);
    $row[] = l(t('remove'), 'admin/content/types/'. $type['url_str'] .'/fields/'. $field['field_name'] .'/remove');

    $rows[] = $row;
  }

  $output = theme('table', $header, $rows);

  return $output;
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

function _content_admin_field_add_existing($type_name) {
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

function _content_admin_field_add_new($type_name) {
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
  }
  else {
    drupal_set_message(t('No field modules are enabled. You need to <a href="%modules_url">enable one</a>, such as text.module, before you can add new fields.', array('!modules_url' => url('admin/modules'))), 'error');
  }

  return $form;
}

function theme_content_admin_field_add_new_field_widget_type($form) {
  $field_types = _content_field_types();
  $widget_types = _content_widget_types();
  $output = '';

  $output .= '<dl>';
  foreach ($field_types as $field_name => $field_type) {
    $output .= '<dt>'. $field_type['label'] .'</dt>';
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
function _content_admin_field_add_existing_submit($form_id, $form_values) {
  $type  = content_types($form_values['type_name']);
  $field = content_fields($form_values['field_name']);
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $columns = module_invoke($field_type['module'], 'field_settings', 'database columns', $field);

  if (is_array($columns) && count($columns)) {
    if ($field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
      $new_field = $field;
      $new_field['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;
      db_query("UPDATE {node_field} SET db_storage = %d WHERE field_name = '%s'", CONTENT_DB_STORAGE_PER_FIELD, $form_values['field_name']);
      content_alter_db_field($field, $columns, $new_field, $columns);
    }
  }

  $prior_instance = db_fetch_array(db_query("SELECT weight, label, widget_type, widget_settings, description FROM {node_field_instance} WHERE field_name = '%s'", $form_values['field_name']));
  if (!$prior_instance) {
    $prior_instance = array();
    $prior_instance['weight'] = 0;
    $prior_instance['label'] = $form_values['field_name'];
    $prior_instance['widget_type'] = '';
    $prior_instance['widget_settings'] = '';
    $prior_instance['description'] = '';
  }
  db_query("INSERT INTO {node_field_instance} (field_name, type_name, weight, label, widget_type, widget_settings, description) VALUES ('%s', '%s', %d, '%s', '%s', '%s', '%s')", $form_values['field_name'], $form_values['type_name'], $prior_instance['weight'], $prior_instance['label'], $prior_instance['widget_type'], $prior_instance['widget_settings'], $prior_instance['description']);

  drupal_set_message(t('Added field %label.', array('%label' => $prior_instance['label'])));
  content_clear_type_cache();
  return 'admin/content/types/'. $type['url_str'] .'/fields';
}

/**
 * Create a new field for a content type.
 */
function _content_admin_field_add_new_submit($form_id, $form_values) {
  // Find a valid, computer-friendly field name.

  $fields = content_fields();
  $type   = content_types($form_values['type_name']);

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
  db_query("INSERT INTO {node_field} (field_name, type, global_settings, required, multiple, db_storage) VALUES ('%s', '%s', '%s', %d, %d, %d)", $field_name, $field_widget_type[0], serialize(array()), 0, 0, CONTENT_DB_STORAGE_PER_CONTENT_TYPE);
  db_query("INSERT INTO {node_field_instance} (field_name, type_name, weight, label, widget_type, widget_settings, description) VALUES ('%s', '%s', %d, '%s', '%s', '%s', '%s')", $field_name, $form_values['type_name'], 0, $form_values['label'], $field_widget_type[1], serialize(array()), '');

  content_clear_type_cache();

  // Create new database columns as necessary.
  $field_types = _content_field_types();
  $field_type = $field_types[$field_widget_type[0]];
  $field = content_fields($field_name);

  $columns = module_invoke($field_type['module'], 'field_settings', 'database columns', $field);
  if (is_array($columns) && count($columns)) {
    content_alter_db_field(array(), array(), $field, $columns);
  }

  drupal_set_message(t('Created field %label.', array('%label' => $form_values['label'])));
  return 'admin/content/types/'. $type['url_str'] .'/fields/'. $field_name;
}

/**
 * Menu callback; present a form for removing a field from a content type.
 */
function _content_admin_field_remove($type_name, $field_name) {
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
    t('Are you sure you want to remove the field %field?', array('%field' => $field['widget']['label'])),
    'admin/content/types/'. $type_name .'/fields',
    t('If you have any content left in this field, it will be lost. This action cannot be undone.'),
    t('Remove'), t('Cancel'),
    '_content_admin_field_remove'
  );

  return $output;
}

/**
 * Remove a field from a content type.
 */
function _content_admin_field_remove_submit($form_id, $form_values) {
  $type = content_types($form_values['type_name']);
  $field = $type['fields'][$form_values['field_name']];

  if ($type && $field && $form_values['confirm']) {
    content_field_instance_delete($form_values);

    drupal_set_message(t('Removed field %field from %type.', array('%field' => theme('placeholder', $field['widget']['label']), '%type' => theme('placeholder', $type['label']))));

    return 'admin/content/types/'. $type['url_str'] .'/fields';
  }
}


/**
 * Menu callback; presents the field editing page.
 */
function _content_admin_field($type_name, $field_name) {
  $output = '';
  
  $type = content_types($type_name);
  $field = $type['fields'][$field_name];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$field['widget']['type']];

  $form = array();
  $form['widget'] = array(
    '#type' => 'fieldset',
    '#title' => t('Widget settings'),
    '#description' => t('These settings apply only to the %field field as it appears in the %type content type.', array('%field' => $field['widget']['label'], '%type' => $type['label'])),
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

  $form['field'] = array(
    '#type' => 'fieldset',
    '#title' => t('Data settings'),
    '#description' => t('These settings apply to the %field field in every content type in which it appears.', array('%field' => $field['widget']['label'])),
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

  return $form;
}

/**
 * Validate a field's settings.
 */
function _content_admin_field_validate($form_id, $form_values) {
  $type = content_types($form_values['type_name']);
  $field = $type['fields'][$form_values['field_name']];
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
  $type = content_types($form_values['type_name']);
  $field = $type['fields'][$form_values['field_name']];
  $field_types = _content_field_types();
  $field_type = $field_types[$field['type']];
  $widget_types = _content_widget_types();
  $widget_type = $widget_types[$form_values['widget_type']];

  $widget_settings = array();
  $setting_names = module_invoke($widget_type['module'], 'widget_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $widget_settings[$setting] = $form_values[$setting];
    }
  }
  $field_settings = array();
  $setting_names = module_invoke($field_type['module'], 'field_settings', 'save', $field);
  if (is_array($setting_names)) {
    foreach ($setting_names as $setting) {
      $field_settings[$setting] = $form_values[$setting];
    }
  }

  $prev_field = $field;
  $prev_columns = module_invoke($field_type['module'], 'field_settings', 'database columns', $field);

  db_query("UPDATE {node_field_instance} SET weight = %d, label = '%s', widget_type = '%s', widget_settings = '%s', description = '%s' WHERE type_name = '%s' AND field_name = '%s'", $form_values['weight'], $form_values['label'], $form_values['widget_type'], serialize($widget_settings), $form_values['description'], $form_values['type_name'], $form_values['field_name']);

  if ($form_values['multiple']) {
    $field['db_storage'] = CONTENT_DB_STORAGE_PER_FIELD;
  }
  else {
    $instances = db_result(db_query("SELECT COUNT(*) FROM {node_field_instance} WHERE field_name = '%s'", $form_values['field_name']));
    if ($instances == 1) {
      $field['db_storage'] = CONTENT_DB_STORAGE_PER_CONTENT_TYPE;
    }
  }
  db_query("UPDATE {node_field} SET required = %d, multiple = %d, global_settings = '%s', db_storage = %d WHERE field_name = '%s'", $form_values['required'], $form_values['multiple'], serialize($field_settings), $field['db_storage'], $form_values['field_name']);

  drupal_set_message(t('Saved field %field.', array('%field' => $form_values['label'])));
  content_clear_type_cache();

  $new_field = content_fields($form_values['field_name']);
  $new_columns = module_invoke($field_type['module'], 'field_settings', 'database columns', $new_field);

  if (!isset($prev_columns)) {
    $prev_columns = array();
  }
  if (!isset($new_columns)) {
    $new_columns = array();
  }
  content_alter_db_field($prev_field, $prev_columns, $new_field, $new_columns);

  return 'admin/content/types/'. $type['url_str'] .'/fields';
}


/**
 * Perform adds, alters, and drops as needed to synchronize the database with
 * new field definitions.
 */
function content_alter_db_field($previous_field, $previous_columns, $new_field, $new_columns) {
  // When adding and removing columns, we need to know what content type has an instance of the field.
  if (count($previous_columns)) {
    if (!isset($previous_field['type_name'])) {
      $previous_field['type_name'] = db_result(db_query("SELECT type_name FROM {node_field_instance} WHERE field_name = '%s'", $previous_field['field_name']));
    }
    $previous_db_info = content_database_info($previous_field);
  }
  if (count($new_columns)) {
    $new_field['type_name'] = db_result(db_query("SELECT type_name FROM {node_field_instance} WHERE field_name = '%s'", $new_field['field_name']));
    $new_db_info = content_database_info($new_field);
  }

  if (!count($new_columns)) {
    if (count($previous_columns)) {
      if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
        db_query('DROP TABLE {'. $previous_db_info['table'] .'}');
      }
      else {
        foreach ($previous_db_info['columns'] as $column => $attributes) {
          db_query('ALTER TABLE {'. $previous_db_info['table'] .'} DROP '. $attributes['column']);
        }
      }
    }
    return;
  }
  
  if ($new_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
    if (!count($previous_columns) || $previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
      // New columns with per-field storage; need to add a table.
      if ($new_field['multiple']) {
        switch ($GLOBALS['db_type']) {
          case 'mysql':
          case 'mysqli':
            db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                vid int unsigned NOT NULL default '0',
                delta int unsigned NOT NULL default '0',
                nid int unsigned NOT NULL default '0',
                PRIMARY KEY (vid,delta)
              ) TYPE=MyISAM /*!40100 DEFAULT CHARACTER SET utf8 */;");
            break;

          case 'pgsql':
            db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                vid integer unsigned NOT NULL default '0',
                delta integer unsigned NOT NULL default '0',
                nid integer unsigned NOT NULL default '0',
                PRIMARY KEY (vid,delta)
              )");
            break;
        }
      }
      else {
        switch ($GLOBALS['db_type']) {
          case 'mysql':
          case 'mysqli':
            db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                vid int unsigned NOT NULL default '0',
                nid int unsigned NOT NULL default '0',
                PRIMARY KEY (vid)
              ) TYPE=MyISAM /*!40100 DEFAULT CHARACTER SET utf8 */;");
            break;

          case 'pgsql':
            db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                vid integer unsigned NOT NULL default '0',
                nid integer unsigned NOT NULL default '0',
                PRIMARY KEY (vid)
              )");
            break;
        }
      }
    }

    if (count($previous_columns) && $previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
      // Already using per-field storage; change multiplicity if needed.
      if ($previous_field['multiple'] && !$new_field['multiple']) {
        db_query('DELETE FROM {'. $new_db_info['table'] .'} WHERE delta != 0');
        db_query('ALTER TABLE {'. $new_db_info['table'] .'} DROP delta');
        switch ($GLOBALS['db_type']) {
          case 'mysql':
          case 'mysqli':
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} DROP PRIMARY KEY');
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} ADD PRIMARY KEY (vid)');
            break;
          case 'pgsql':
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} DROP CONSTRAINT {'. $new_db_info['table'] .'}_pkey');
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} ADD PRIMARY KEY (vid)');
            break;
        }
      }
      else if (!$previous_field['multiple'] && $new_field['multiple']) {
        content_db_add_column($new_db_info['table'], 'delta', 'int', array('unsigned' => TRUE, 'not null' => TRUE, 'default' => 0));
        switch ($GLOBALS['db_type']) {
          case 'mysql':
          case 'mysqli':
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} DROP PRIMARY KEY');
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} ADD PRIMARY KEY (vid,delta)');
            break;
          case 'pgsql':
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} DROP CONSTRAINT {'. $new_db_info['table'] .'}_pkey');
            db_query('ALTER TABLE {'. $new_db_info['table'] .'} ADD PRIMARY KEY (vid,delta)');
            break;
        }
      }
    }
  }

  // Add new columns and change modified columns.
  foreach ($new_columns as $column => $attributes) {
    $column_name = $new_field['field_name'] .'_'. $column;
    if (!isset($previous_columns[$column]) || $previous_field['db_storage'] != $new_field['db_storage']) {
      if (!db_table_exists($new_db_info['table'])) {
        if ($new_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
          if ($new_field['multiple']) {
            switch ($GLOBALS['db_type']) {
              case 'mysql':
              case 'mysqli':
                db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                  vid int unsigned NOT NULL default '0',
                  delta int unsigned NOT NULL default '0',
                  nid int unsigned NOT NULL default '0',
                  PRIMARY KEY (vid,delta)
                ) TYPE=MyISAM /*!40100 DEFAULT CHARACTER SET utf8 */;");
                break;

              case 'pgsql':
                db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                  vid integer unsigned NOT NULL default '0',
                  delta integer unsigned NOT NULL default '0',
                  nid integer unsigned NOT NULL default '0',
                  PRIMARY KEY (vid,delta)
                )");
                break;
            }

          }
          else {
            switch ($GLOBALS['db_type']) {
              case 'mysql':
              case 'mysqli':
                db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                  vid int unsigned NOT NULL default '0',
                  nid int unsigned NOT NULL default '0',
                  PRIMARY KEY (vid)
                ) TYPE=MyISAM /*!40100 DEFAULT CHARACTER SET utf8 */;");
                break;

                case 'pgsql':
                  db_query("CREATE TABLE {". $new_db_info['table'] ."} (
                    vid integer unsigned NOT NULL default '0',
                    nid integer unsigned NOT NULL default '0',
                    PRIMARY KEY (vid)
                  )");
                  break;
            }
          }  // end: if ($new_field['multiple'])
        }  // end: if ($new_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE)
      }      content_db_add_column($new_db_info['table'], $column_name, $attributes['type'], $attributes);
    }
    else {
      if ($attributes != $previous_columns[$column]) {
        content_db_change_column($new_db_info['table'], $column_name, $column_name, $attributes['type'], $attributes);
      }
    }
  }
  
  if (count($previous_columns) && count($new_columns)) {
    // Remove obsolete columns.
    foreach ($previous_columns as $column => $attributes) {
      $column_name = $previous_field['field_name'] .'_'. $column;
      if (!isset($new_columns[$column])) {
        db_query('ALTER TABLE {'. $new_db_info['table'] .'} DROP '. $column_name);
      }
    }

    // Migrate data from per-content-type storage.
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE && $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD) {
      $columns = array();
      foreach ($previous_db_info['columns'] as $column => $attributes) {
        $columns[] = $attributes['column'];
      }
      if ($new_field['multiple']) {
        db_query('INSERT INTO {'. $new_db_info['table'] .'} (vid, nid, delta, '. implode(', ', $columns) .') SELECT vid, nid, 0, '. implode(', ', $columns) .' FROM {'. $previous_db_info['table'] .'}');
      }
      else {
        db_query('INSERT INTO {'. $new_db_info['table'] .'} (vid, nid, '. implode(', ', $columns) .') SELECT vid, nid, '. implode(', ', $columns) .' FROM {'. $previous_db_info['table'] .'}');
      }
      foreach ($columns as $column_name) {
        db_query('ALTER TABLE {'. $previous_db_info['table'] .'} DROP '. $column_name);
      }
    }

    // Migrate data from per-field storage.
    if ($previous_field['db_storage'] == CONTENT_DB_STORAGE_PER_FIELD && $new_field['db_storage'] == CONTENT_DB_STORAGE_PER_CONTENT_TYPE) {
      $column_names = array();
      $column_placeholders = array();
      $column_assignments = array();
      foreach ($new_db_info['columns'] as $column => $attributes) {
        $column_names[] = $attributes['column'];
        if (in_array($attributes['type'], array('int', 'mediumint', 'tinyint', 'bigint', 'float'))) {
          $column_placeholders[] = '%d';
          $column_assignments[] = $attributes['column'] .' = %d';
        }
        else {
          $column_placeholders[] = "'%s'";
          $column_assignments[] = $attributes['column'] ." = '%s'";
        }
      }
      if ($previous_field['multiple']) {
        $result = db_query('SELECT '. implode(', ', $column_names) .', vid, nid FROM {'. $previous_db_info['table'] .'} WHERE delta = 0');
      }
      else {
        $result = db_query('SELECT '. implode(', ', $column_names) .', vid, nid FROM {'. $previous_db_info['table'] .'}');
      }
      while ($data = db_fetch_array($result)) {
        if (db_result(db_query('SELECT COUNT(*) FROM {'. $new_db_info['table'] .'} WHERE vid = %d AND nid = %d', $data['vid'], $data['nid']))) {
          db_query('UPDATE {'. $new_db_info['table'] .'} SET '. implode(', ', $column_assignments) .' WHERE vid = %d AND nid = %d', $data);
        }
        else {
          db_query('INSERT INTO {'. $new_db_info['table'] .'} ('. implode(', ', $column_names) .', vid, nid) VALUES ('. implode(', ', $column_placeholders) .', %d, %d)', $data);
        }
      }
      db_query('DROP TABLE {'. $previous_db_info['table'] .'}');
    }
  }
}

/**
 * Add a column to a database table.
 *
 * @param $table
 *   Name of the table, without {}
 * @param $column
 *   Name of the column
 * @param $type
 *   Type of column
 * @param $attributes
 *   Additional optional attributes. Recognized attributes:
 *     not null => TRUE|FALSE
 *     default  => NULL|FALSE|value (with or without '', it won't be added)
 */
function content_db_add_column($table, $column, $type, $attributes = array()) {
  switch ($GLOBALS['db_type']) {
    case 'pgsql':
      $mappings = array('int' => 'integer', 'mediumint' => 'integer', 'bigint' => 'integer',
        'tinyint' => 'smallint',
        'float' => 'float',
        'varchar' => 'varchar',
        'text' => 'text', 'mediumtext' => 'text', 'longtext' => 'text');
      if (isset($mappings[$type])) {
        $type = $mappings[$type];
      }
      else {
        watchdog('database', t('No PostgreSQL mapping found for %type data type.', array('%type' => $type)), WATCHDOG_WARNING);
      }
      if ($type != 'varchar') {
        unset($attributes['length']);
      }
      break;

    case 'mysql':
    case 'mysqli':
      break;
  }

  if (array_key_exists('not null', $attributes) && $attributes['not null']) {
    $not_null = 'NOT NULL';
  }
  if (array_key_exists('default', $attributes)) {
    if (is_null($attributes['default'])) {
      $default_val = 'NULL';
      $default = 'default NULL';
    }
    elseif ($attributes['default'] === FALSE) {
      $default = '';
    }
    else {
      $default_val = "$attributes[default]";
      $default = "default $attributes[default]";
    }
  }
  if (array_key_exists('length', $attributes)) {
    $type .= '('. $attributes['length'] .')';
  }
  if (array_key_exists('unsigned', $attributes) && $attributes['unsigned']) {
    $type .= ' unsigned';
  }

  switch ($GLOBALS['db_type']) {
    case 'pgsql':
      db_query("ALTER TABLE {". $table ."} ADD $column $type");
      if ($default) {
        db_query("ALTER TABLE {". $table ."} ALTER $column SET $default");
      }
      if ($not_null) {
        if ($default) {
          db_query("UPDATE {". $table ."} SET $column = $default_val");
        }
        db_query("ALTER TABLE {". $table ."} ALTER $column SET NOT NULL");
      }
      break;

    case 'mysql':
    case 'mysqli':
      db_query('ALTER TABLE {'. $table .'} ADD COLUMN '. $column .' '. $type .' '. $not_null .' '. $default);
      break;
  }
}

/**
 * Change a column definition.
 *
 * Remember that changing a column definition involves adding a new column
 * and dropping an old one. This means that any indices, primary keys and
 * sequences from serial-type columns are dropped and might need to be
 * recreated.
 *
 * @param $table
 *   Name of the table, without {}
 * @param $column
 *   Name of the column to change
 * @param $column_new
 *   New name for the column (set to the same as $column if you don't want to change the name)
 * @param $type
 *   Type of column
 * @param $attributes
 *   Additional optional attributes. Recognized attributes:
 *     not null => TRUE|FALSE
 *     default  => NULL|FALSE|value (with or without '', it won't be added)
 */
function content_db_change_column($table, $column, $column_new, $type, $attributes = array()) {
  switch ($GLOBALS['db_type']) {
    case 'pgsql':
      $mappings = array('int' => 'integer', 'mediumint' => 'integer', 'bigint' => 'integer',
        'tinyint' => 'smallint',
        'float' => 'float',
        'varchar' => 'varchar',
        'text' => 'text', 'mediumtext' => 'text', 'longtext' => 'text');
      if (isset($mappings[$type])) {
        $type = $mappings[$type];
      }
      else {
        watchdog('database', t('No PostgreSQL mapping found for %type data type.', array('%type' => $type)), WATCHDOG_WARNING);
      }
      if ($type != 'varchar') {
        unset($attributes['length']);
      }
      break;

    case 'mysql':
    case 'mysqli':
      break;
  }

  if (array_key_exists('not null', $attributes) and $attributes['not null']) {
    $not_null = 'NOT NULL';
  }
  if (array_key_exists('default', $attributes)) {
    if (is_null($attributes['default'])) {
      $default_val = 'NULL';
      $default = 'default NULL';
    }
    elseif ($attributes['default'] === FALSE) {
      $default = '';
    }
    else {
      $default_val = "$attributes[default]";
      $default = "default $attributes[default]";
    }
  }
  if (array_key_exists('length', $attributes)) {
    $type .= '('. $attributes['length'] .')';
  }
  if (array_key_exists('unsigned', $attributes) && $attributes['unsigned']) {
    $type .= ' unsigned';
  }

  switch ($GLOBALS['db_type']) {
    case 'pgsql':
      db_query("ALTER TABLE {". $table ."} RENAME $column TO ". $column ."_old");
      db_query("ALTER TABLE {". $table ."} ADD $column_new $type");
      db_query("UPDATE {". $table ."} SET $column_new = ". $column ."_old");
      if ($default) {
        db_query("ALTER TABLE {". $table ."} ALTER $column_new SET $default");
      }
      if ($not_null) {
        db_query("ALTER TABLE {". $table ."} ALTER $column_new SET NOT NULL");
      }
      db_query("ALTER TABLE {". $table ."} DROP ". $column ."_old");
      break;

    case 'mysql':
    case 'mysqli':
      db_query('ALTER TABLE {'. $table .'} CHANGE '. $column .' '. $column_new .' '. $type .' '. $not_null .' '. $default);
      break;
  }
}
