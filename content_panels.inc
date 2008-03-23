<?php
// $Id$

/**
 * Callback function to supply a list of content types.
 */
function content_panels_content_types() {
  $items = array();
  if (module_exists('fieldgroup')) {
    $items['node_cck_group'] = array(
      'title' => t('Node CCK Group'),
      'content_types' => 'content_admin_content_types_node_cck_group',
      'single' => TRUE, // only provides a single content type
      'render callback' => 'content_content_node_cck_group',
      'add callback' => 'content_admin_edit_node_cck_group',
      'edit callback' => 'content_admin_edit_node_cck_group',
      'title callback' => 'content_admin_title_node_cck_group',
    );
  }
  return $items;
}

/**
 * 'Render' callback for the 'CCK group' content type.
 */
function content_content_node_cck_group($conf, $panel_args, $context) {
  $node = isset($context->data) ? drupal_clone($context->data) : NULL;

  $blok = new stdClass();
  $block->module = 'cck_group';

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
    $block->content = t('Group content goes here.');
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
function content_admin_content_types_node_cck_group() {
  return array(
    'description' => array(
      'title' => t('CCK Group'),
      'icon' => 'icon_node.png',
      'path' => panels_get_path('content_types/node'),
      'description' => t('Group contents.'),
      'required context' => new panels_required_context(t('Node'), 'node'),
      'category' => array(t('Node context'), -9),
    ),
  );
}


/**
 * 'Edit' callback for the 'CCK group' content type.
 */
function content_admin_edit_node_cck_group($id, $parents, $conf = array()) {
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

  $form['group'] = array(
    '#type' => 'select',
    '#title' => t('Which group'),
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
 * 'Title' callback for the 'CCK group' content type.
 */
function content_admin_title_node_cck_group($conf, $context) {
  return t('"@s" node cck group', array('@s' => $context->identifier));
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
