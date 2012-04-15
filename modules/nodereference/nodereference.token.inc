<?php

/**
 * @file
 * Provides tokens for nodereference fields.
 */

/**
 * Implementation of hook_token_list().
 */
function nodereference_token_list($type = 'all') {
  if ($type == 'field' || $type == 'all') {
    $tokens = array();

    $tokens['node reference']['nid']   = t('Referenced node ID');
    $tokens['node reference']['title'] = t('Referenced node title');
    $tokens['node reference']['title-raw'] = t('Referenced node unfiltered title. WARNING - raw user input.');
    $tokens['node reference']['link']  = t("Formatted html link to the referenced node.");
    $tokens['node reference']['path']  = t("Relative path alias to the referenced node.");
    $tokens['node reference']['url']  = t("Absolute path alias to the referenced node.");

    return $tokens;
  }
}

/**
 * Implementation of hook_token_values().
 */
function nodereference_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'field') {
    $item = $object[0];

    $title = is_numeric($item['nid']) ? _nodereference_titles($item['nid']) : '';
    $tokens['nid']   = $item['nid'];
    $tokens['title'] = $title ? check_plain($title) : '';
    $tokens['title-raw'] = $title;
    $tokens['link']  = isset($item['view']) ? $item['view'] : '';
    $tokens['path'] = is_numeric($item['nid']) ? url('node/' . $item['nid']) : '';
    $tokens['url'] = is_numeric($item['nid']) ? url('node/' . $item['nid'], array('absolute' => TRUE)) : '';

    return $tokens;
  }
}
