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
  $block->module = 'cck_group';

  if ($node) {
    // Assemble the fields into groups
    $node = node_build_content($node);
  
    // Get the "machine name" of the group from the options
    $groupname = $conf['group'];

    // Set the subject to the nice name of the group
    $block->subject = $node->content[$groupname]['#title'];
    
    // Print out the contents of the given group
    // Note, not using drupal_render($node->content[$groupname]) here to avoid printing the fieldset
    if (is_array($node->content[$groupname])) {
      foreach ($node->content[$groupname] as $key => $value) {
        if (is_array($value)) {
          if (array_key_exists('#value', $value)) {
            $output .= $value['#value'] ;
          }
        }
      }
    }
    
    if (empty($output)) {
      // Output the given empty text
      $output = $conf['empty'];
    }
    $block->content = $output;   
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
 * Helper function : build the list of groups for the 'edit' form. 
 */
function _content_admin_node_cck_group_options() {
  $group_list = array();
  
  // Retrieve the list of all groups on all content types
  $types = module_exists('fieldgroup') ? fieldgroup_groups(NULL, FALSE, FALSE) : array();
  
  // Add each group to the list with the content type it is from in parentheses
  foreach ($types as $type) {  
    foreach ($type as $group) {
      $group_list[$group['group_name']] = $group['label'] . ' (' . $group['type_name'] . ')';
    }
  }

  return $group_list;
}

/**
 * 'Edit' callback for the 'CCK group' content type. 
 */
function content_admin_edit_node_cck_group($id, $parents, $conf = array()) {
  // Apply defaults
  if (empty($conf)) {
    $conf = array('title' => '', 'group' => '', 'empty' => '');
  }

  $form['group'] = array(
    '#type' => 'select',
    '#title' => t('Which group'),
    '#options' => _content_admin_node_cck_group_options(),
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
