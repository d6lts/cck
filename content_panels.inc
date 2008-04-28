<?php
// $Id$
/**
 * Panels 2 interface for CCK fieldgroups and fields.
 * 
 * TODO:
 * 
 * - Adjust the fields and groups that are displayed for context, when that
 *   capability is added to Panels.
 * 
 */

/**
 * Implementation of hook_panels_content_types()
 */
function content_panels_content_types() {
  $items = array();
  $items['content_field'] = array(
    'title' => t('Content field'),
    'single' => TRUE,
    'content_types' => 'content_panels_content_types_field',
    'add callback' => 'content_panels_edit_field',
    'edit callback' => 'content_panels_edit_field',
    'render callback' => 'content_panels_render_field',
    'title callback' => 'content_panels_title_content',
    );
  if (module_exists('fieldgroup')) {
    $items['content_fieldgroup'] = array(
      'title' => t('Content fieldgroup'),
      'content_types' => 'content_panels_content_types_node_fieldgroup',
      'single' => TRUE, // only provides a single content type
      'render callback' => 'content_panels_node_fieldgroup',
      'add callback' => 'content_panels_edit_node_fieldgroup',
      'edit callback' => 'content_panels_edit_node_fieldgroup',
      'title callback' => 'content_panels_title_node_fieldgroup',
    );
  }
  return $items;
}

function content_panels_content_types_field() {
  return array(
    'description' => array(
      'title' => t('Content field'),
      'icon' => 'icon_node.png',
      'path' => panels_get_path('content_types/node'),
      'description' => t('A content field from the referenced node.'),
      'required context' => new panels_required_context(t('Node'), 'node'),
      'category' => array(t('Node context'), -9),
    ),
  );
}

function content_panels_edit_field($id, $parents, $conf = array()) {

  $form = array();
  $form['label'] = array(
    '#type' => 'select',
    '#title' => t('Label'),
    '#default_value' => $conf['label'],
    '#options' => array(
      'normal' => t('Block title'),
      'above' => t('Above'),
      'inline' => t('Inline'),
      'hidden' => t('hidden'),
    ),
    '#description' => t('Configure how the label is going to be displayed'),
  );

  
  $options = array('' => '');
  $fields = content_fields();
  $field_types = _content_field_types();
  foreach ($fields as $field_name => $field) {
    $type_info = $field_types[$field['type']];
    foreach ($type_info['formatters'] as $formatter_name => $formatter) {
      $label  = $type_info['label'] .':'. $field['widget']['label'] ;
      $label .= $field['widget']['label'] != $field_name ? ' ('. $field_name .')' : '';
      $options[$label][$field_name .':'. $formatter_name] = $formatter['label'];
    }
  }
  ksort($options);
  $form['field_formatter'] = array(
    '#type' => 'select',
    '#title' => t('Field / Formatter'),
    '#default_value' => $conf['field_formatter'],
    '#options' => $options,
    '#description' => t('Select a field and formatter.'),
    '#required' => TRUE,
  );
  return $form;
}

/**
 * 'Title' callback for the 'field' content type.
 */
function content_panels_title_content($conf, $context) {
  $data =explode(':', $conf['field_formatter']);
  $field_name = $data[0];
  $formatter = $data[1];
  $fields = content_fields();
  $field = $fields[$field_name];
  $field_types = _content_field_types();
  return t('"@s" field @name', array('@s' => $context->identifier, '@name' => $field_types[$field['type']]['label'] .': '. $field['widget']['label'] .' ('. $field['field_name'])) .')';
}

function content_panels_render_field($conf, $panel_args, $context) {
  if (!empty($context) && empty($context->data)) {
    return;
  }

  $node = isset($context->data) ? drupal_clone($context->data) : NULL;
  $info = _content_type_info($id);
  $data = explode(':', $conf['field_formatter']);
  $field_name = $data[0];
  $formatter = $data[1];
  
  $field = $info['fields'][$field_name];
  $field_info = $info['field types'][$field['type']];

  $field['display_settings']['label']['format'] = $conf['title_formatter'] == 'normal' ? 'hidden' : $conf['title_formatter'];
  $field['display_settings']['full']['format'] = $formatter;

  $block->module = 'content';
  $block->delta = $conf['field_name'];

  if ($conf['label'] == 'normal') {
    $block->title = $field['widget']['label'];
  }
  $node_field = isset($node->$field['field_name']) ? $node->$field['field_name'] : array();

  if (content_handle('field', 'view', $field) == CONTENT_CALLBACK_CUSTOM) {
    $field_types = _content_field_types();
    $module = $field_types[$field['type']]['module'];
    $function = $module .'_field';
    if (function_exists($function)) {
      $value = $function('view', $node, $field, $node_field, 0, 0);
    }
  }
  else {
    foreach ($node_field as $delta => $item) {
      $node_field[$delta]['view'] = content_format($field, $item, $conf['formatter'], $node);
    }
    $value = theme('field', $node, $field, $node_field, 0, 0);
  }
  $block->content = $value;

  return $block;
}


/**
 * 'Render' callback for the 'fieldgroup' content type.
 */
function content_panels_node_fieldgroup($conf, $panel_args, $context) {
  $node = isset($context->data) ? drupal_clone($context->data) : NULL;

  $block = new stdClass();
  $block->module = 'content';

  if ($node) {
    // Assemble the fields into groups
    $node = node_build_content($node);

    // Get the "machine name" of the group from the options
    $groupname = $conf['group'];

    // Print out the contents of the given group
    // Note, not using drupal_render($node->content[$groupname]) here to avoid printing the fieldset
    $vars = array();
    if (is_array($node->content[$groupname])) {
      foreach (element_children($node->content[$groupname]) as $key) {
        $vars[$key] = drupal_render($node->content[$groupname][$key]);
      }
    }

    $block->subject = $node->content[$groupname]['#title'];
    $block->content = $vars ? theme('panels_content_group', $vars, $node->nid) : $conf['empty'];
    $block->delta = $node->nid;
  }
  else {
    $block->subject = $conf['group'];
    $block->content = t('Content fieldgroup content goes here.');
    $block->delta = 'unknown';
  }

  return $block;
}

/**
 * Allows users to theme the group
 */
function theme_panels_content_group($vars, $nid) {
  return implode('', $vars);
}

/**
 * Return all content types available.
 */
function content_panels_content_types_node_fieldgroup() {
  return array(
    'description' => array(
      'title' => t('Content fieldgroup'),
      'icon' => 'icon_node.png',
      'path' => panels_get_path('content_types/node'),
      'description' => t('All fields from a fieldgroup on the referenced node.'),
      'required context' => new panels_required_context(t('Node'), 'node'),
      'category' => array(t('Node context'), -9),
    ),
  );
}

/**
 * 'Edit' callback for the 'fieldgroup' content type.
 */
function content_panels_edit_node_fieldgroup($id, $parents, $conf = array()) {
  // Apply defaults
  if (empty($conf)) {
    $conf = array('title' => '', 'group' => '', 'empty' => '');
  }

  // Retrieve the list of all groups on all content types
  $group_list = array();
  $types = module_exists('fieldgroup') ? fieldgroup_groups(NULL, FALSE, FALSE) : array();
  // Add each group to the list with the content type it is from in parentheses
  foreach ($types as $type) {
    foreach ($type as $group) {
      $group_list[$group['group_name']] = $group['label'] . ' (' . $group['type_name'] . ')';
    }
  }

  $form['type_name'] = array(
    '#type' => 'value',
    '#value' => $group['type_name'],
  );

  $form['group'] = array(
    '#type' => 'select',
    '#title' => t('Fieldgroup'),
    '#options' => $group_list,
    '#default_value' => $conf['group'],
    '#prefix' => '<div class="clear-block no-float">',
    '#suffix' => '</div>',
  );

  $form['empty'] = array(
    '#type' => 'textarea',
    '#title' => 'Empty text',
    '#description' => t('Text to display if group has no data. Note that title will not display unless overridden.'),
    '#rows' => 5,
    '#default_value' => $conf['empty'],
    '#prefix' => '<div class="clear-block no-float">',
    '#suffix' => '</div>',
  );

  return $form;
}

/**
 * 'Title' callback for the 'fieldgroup' content type.
 */
function content_panels_title_node_fieldgroup($conf, $context) {
  $types = module_exists('fieldgroup') ? fieldgroup_groups(NULL, FALSE, FALSE) : array();
  $type = $types[$conf['type_name']][$conf['group']];
  return t('"@s" fieldgroup @name', array('@s' => $context->identifier, '@name' => $type['label']));
}


function content_panels_relationships() {
  $args = array();
  if (module_exists('nodereference')) {
    $args['node_from_noderef'] = array(
      'title' => t('Node from reference'),
      'keyword' => 'nodereference',
      'description' => t('Adds a node from a node reference in a node context; if multiple nodes are referenced, this will get the first referenced node only.'),
      'required context' => new panels_required_context(t('Node'), 'node'),
      'context' => 'content_node_from_noderef_context',
      'settings form' => 'content_node_from_noderef_settings_form',
      'settings form validate' => 'content_node_from_noderef_settings_form_validate',
    );
  }
  if (module_exists('userreference')) {
    $args['user_from_userref'] = array(
      'title' => t('User from reference'),
      'keyword' => 'userreference',
      'description' => t('Adds a user from a user reference in a node context; if multiple users are referenced, this will get the first referenced user only.'),
      'required context' => new panels_required_context(t('Node'), 'node'),
      'context' => 'content_user_from_userref_context',
      'settings form' => 'content_usere_from_userref_settings_form',
      'settings form validate' => 'content_user_from_userref_settings_form_validate',
    );
  }
  return $args;
}

/**
 * Return a new context based on an existing context
 */
function content_node_from_noderef_context($context = NULL, $conf) {
  // If unset it wants a generic, unfilled context, which is just NULL
  if (empty($context->data)) {
    return panels_context_create_empty('node', NULL);
  }

  if (isset($context->data->{$conf['field_name']}[0]['nid']) && ($nid = $context->data->{$conf['field_name']}[0]['nid'])) {
    if ($node = node_load($nid)) {
      return panels_context_create('node', $node);
    }
  }
}

/**
 * Settings form for the relationship
 */
function content_node_from_noderef_settings_form($conf) {
  $options = array();
  foreach (content_fields() as $field) {
    if ($field['type'] == 'nodereference') {
      $options[$field['field_name']] = t($field['widget']['label']);
    }
  }
  $form['field_name'] = array(
    '#title' => t('Node reference field'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $conf['field_name'],
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
  );

  return $form;
}

/**
 * Return a new context based on an existing context
 */
function content_user_from_userref_context($context = NULL, $conf) {
  // If unset it wants a generic, unfilled context, which is just NULL
  if (empty($context->data)) {
    return panels_context_create_empty('user', NULL);
  }

  if (isset($context->data->{$conf['field_name']}[0]['uid']) && ($uid = $context->data->{$conf['field_name']}[0]['uid'])) {
    if ($account = user_load(array('uid' => $uid))) {
      return panels_context_create('user', $account);
    }
  }
}

/**
 * Settings form for the relationship
 */
function content_user_from_userref_context_settings_form($conf) {
  $options = array();
  foreach (content_fields() as $field) {
    if ($field['type'] == 'userreference') {
      $options[$field['field_name']] = t($field['widget']['label']);
    }
  }
  $form['field_name'] = array(
    '#title' => t('User reference field'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $conf['field_name'],
    '#prefix' => '<div class="clear-block">',
    '#suffix' => '</div>',
  );

  return $form;
}

