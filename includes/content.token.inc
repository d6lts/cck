<?php

/**
 * @file
 * Implementation of hook_content_build_modes
 * (on behalf of token.module)
 */
function token_content_build_modes() {
  return array(
    'token' => array(
      'title' => t('Token'),
      'build modes' => array(
        'token' => array(
          'title' => t('Token'),
          'views style' => FALSE,
        ),
      ),
    ),
  );
}

// Two helper functions that generate appropriate tokens for CCK-added fields.
function content_token_values($type, $object = NULL, $options = array()) {
  $tokens = array();
  if ($type == 'node') {
    // Prevent against invalid 'nodes' built by broken 3rd party code.
    if (isset($object->type)) {
      // Let PHP free the $node object when we are done. Working directly on the
      // incoming $object causes memory leak issues on long-running scripts such
      // as migrations. See http://drupal.org/node/736440.
      $node = drupal_clone($object);
      $content_type = content_types($node->type);
      $node->build_mode = 'token';
      $node->content = array();
      content_view($node);
      // The formatted values will only be known after the content has been rendered.
      drupal_render($node->content);
      content_alter($node);

      $field_types = _content_field_types();
      foreach ($content_type['fields'] as $field_name => $field) {
        $items = isset($node->{$field_name}) ? $node->{$field_name} : array();
        $function = $field_types[$field['type']]['module'] . '_token_values';
        if (!empty($items) && function_exists($function)) {
          $token_values = (array) $function('field', $items, $options);
          foreach ($token_values as $token => $value) {
            $tokens[$field_name .'-'. $token] = $value;
          }
        }
      }
    }
  }
  return $tokens;
}

function content_token_list($type = 'all') {
  if ($type == 'node' || $type == 'all') {
    $list = array();
    $field_types = _content_field_types();

    foreach (content_fields() as $field) {
      $sub_list = array();
      $function = $field_types[$field['type']]['module'] . '_token_list';
      if (function_exists($function)) {
        $sub_list = $function('field');
        foreach ($sub_list as $category => $token_list) {
          foreach ($token_list as $token => $description) {
            $list['CCK '. $category][$field['field_name'] .'-'. $token] = $description;
          }
        }
      }
    }

    return $list;
  }
}
