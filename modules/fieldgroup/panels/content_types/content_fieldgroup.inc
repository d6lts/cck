<?php
// $Id$

/**
 * @file
 * This file provides a CTools content type for fieldgroups.
 */

/**
 * Callback function to supply a list of content types.
 */
function fieldgroup_content_fieldgroup_ctools_content_types() {
  return array(
    'title' => t('Content fieldgroup'),
    'defaults' => array('empty' => ''),
  );
}

/**
 * Return all fieldgroup content types available.
 */
function fieldgroup_content_fieldgroup_content_type_content_types() {
  // This will hold all the individual fieldgroup content types.
  $types = array();

  // The outer loop goes through each node type with groups.
  foreach (fieldgroup_groups() as $node_type_groups) {
    // The inner loop gives us each fieldgroup on each node type with groups.
    foreach ($node_type_groups as $fieldgroup) {
      // Skip field groups that are not of standard type.
      if ($fieldgroup['group_type'] != 'standard') {
        continue;
      }

      // Name the content type a combination of fieldgroup and node type names.
      $content_type_name = $fieldgroup['type_name'] . ':' . $fieldgroup['group_name'];

      // Assemble the information about the content type.
      $info = array(
        'category' => t('Content'),
        'title' => t('@group in @type (standard field group)', array(
          '@group' => t($fieldgroup['label']),
          '@type' => node_get_types('name', $fieldgroup['type_name']),
        )),
        'description' => t('All fields from this field group on the referenced node.'),
        'required context' => new ctools_context_required(t('Node'), 'node', array('type' => array($fieldgroup['type_name']))),
      );

      $types[$content_type_name] = $info;
    }
  }

  return $types;
}

/**
 * Output function for the 'fieldgroup' content type.
 */
function fieldgroup_content_fieldgroup_content_type_render($subtype, $conf, $panel_args, $context) {
  if (!isset($context->data)) {
    return;
  }
  $node = drupal_clone($context->data);

  // Extract the node type and fieldgroup name from the subtype.
  list($node_type, $fieldgroup_name) = explode(':', $subtype, 2);

  // Get a list of all fieldgroups for this node type.
  $groups = fieldgroup_groups($node_type);

  if (!isset($groups[$fieldgroup_name])) {
    return;
  }

  $group = $groups[$fieldgroup_name];
  $output = array();

  foreach ($group['fields'] as $field_name => $field) {
    $field = content_fields($field_name, $node_type);
    $field_view = content_view_field($field, $node);
    if (!is_null($field_view)) {
      $output[] = $field_view;
    }
  }

  $block = new stdClass();
  $block->title = $group['label'];
  $block->content = !empty($output) ? theme('fieldgroup_content_type', $output, $node->nid) : $conf['empty'];

  return $block;
}

/**
 * Allows users to theme the fieldgroup content type.
 */
function theme_fieldgroup_content_type($vars, $nid) {
  return implode('', $vars);
}

/**
 * Returns a settings form for the custom type.
 */
function fieldgroup_content_fieldgroup_content_type_edit_form(&$form, &$form_state) {
  $conf = $form_state['conf'];

  $form['empty'] = array(
    '#type' => 'textarea',
    '#title' => 'Empty text',
    '#description' => t('Text to display if group has no data. Note that title will not display unless overridden.'),
    '#rows' => 5,
    '#default_value' => $conf['empty'],
  );
}

function fieldgroup_content_fieldgroup_content_type_edit_form_submit(&$form, &$form_state) {
  // Copy everything from our defaults.
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    $form_state['conf'][$key] = $form_state['values'][$key];
  }
}

/**
 * Admin title for fieldgroup content type.
 */
function fieldgroup_content_fieldgroup_content_type_admin_title($subtype, $conf, $context) {
  return t('"@s" fieldgroup (@name)', array('@s' => $context->identifier, '@name' => $subtype));
}
