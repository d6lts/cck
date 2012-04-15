<?php

/**
 * @file
 * Provides tokens for userreference fields.
 */

/**
 * Implementation of hook_token_list().
 */
function userreference_token_list($type = 'all') {
  if ($type == 'field' || $type == 'all') {
    $tokens = array();

    $tokens['user reference']['uid']   = t('Referenced user ID');
    $tokens['user reference']['name']  = t('Referenced user name');
    $tokens['user reference']['link']  = t('Formatted HTML link to referenced user');
    $tokens['user reference']['path']  = t("Relative path alias to the referenced user.");
    $tokens['user reference']['url']  = t("Absolute path alias to the referenced user.");

    return $tokens;
  }
}

/**
 * Implementation of hook_token_values().
 */
function userreference_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'field') {
    $item = $object[0];

    $tokens['uid']   = $item['uid'];
    $tokens['name']  = isset($item['view']) ? strip_tags($item['view']) : '';
    $tokens['link']  = isset($item['view']) ? $item['view'] : '';
    $tokens['path'] = is_numeric($item['uid']) ? url('user/' . $item['uid']) : '';
    $tokens['url'] = is_numeric($item['uid']) ? url('user/' . $item['uid'], array('absolute' => TRUE)) : '';

    return $tokens;
  }
}
