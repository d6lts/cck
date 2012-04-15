<?php

/**
 * @file
 * Provides tokens for number fields.
 */

/**
 * Implementation of hook_token_list().
 */
function number_token_list($type = 'all') {
  if ($type == 'field' || $type == 'all') {
    $tokens = array();

    $tokens['number']['raw']       = t('Raw number value');
    $tokens['number']['formatted'] = t('Formatted number value');

    return $tokens;
  }
}

/**
 * Implementation of hook_token_values().
 */
function number_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'field') {
    $item = $object[0];

    $tokens['raw']       = $item['value'];
    $tokens['formatted'] = isset($item['view']) ? $item['view'] : '';

    return $tokens;
  }
}
