<?php

/**
 * @file
 * Provides tokens for text fields.
 */

/**
 * Implementation of hook_token_list().
 */
function text_token_list($type = 'all') {
  if ($type == 'field' || $type == 'all') {
    $tokens = array();

    $tokens['text']['raw']       = t('Raw, unfiltered text');
    $tokens['text']['formatted'] = t('Formatted and filtered text');

    return $tokens;
  }
}

/**
 * Implementation of hook_token_values().
 */
function text_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'field') {
    $item = $object[0];

    $tokens['raw']  = $item['value'];
    $tokens['formatted'] = isset($item['view']) ? $item['view'] : '';
    return $tokens;
  }
}
