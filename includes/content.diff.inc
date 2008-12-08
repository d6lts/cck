<?php
// $Id$

/**
 * @file hook_diff() implementations for CCK (especially fields).
 *
 * These should use a field-hook so the data for the diff is
 * field-type specific.
 */

/**
 * Implementation of hook_diff()
 */
function content_diff(&$old_node, &$new_node) {
  $result = array();
  $cck_info = content_types($new_node->type);
  if ($cck_info) {
    foreach ($cck_info['fields'] as $field) {
      $old_values = array();
      $new_values = array();
      switch ($field['type']) {
        case 'nodereference':
          if (isset($old_node->$field['field_name'])) {
            $old_values = _cck_nodereference_diff_values($old_node, $field);
          }
          if (isset($new_node->$field['field_name'])) {
            $new_values = _cck_nodereference_diff_values($new_node, $field);
          }
          break;
        case 'userreference':
          if (isset($old_node->$field['field_name'])) {
            $old_values = _cck_userreference_diff_values($old_node, $field);
          }
          if (isset($new_node->$field['field_name'])) {
            $new_values = _cck_userreference_diff_values($new_node, $field);
          }
          break;
        case 'number_integer':
        case 'text':
          if (isset($old_node->$field['field_name'])) {
            $old_values = _cck_text_diff_values($old_node, $field);
          }
          if (isset($new_node->$field['field_name'])) {
            $new_values = _cck_text_diff_values($new_node, $field);
          }
          break;
      }
      if ($old_values || $new_values) {
        $result[$field['field_name']] = array(
          '#name' => $field['widget']['label'],
          '#old' => $old_values,
          '#new' => $new_values,
          '#weight' => $field['widget']['weight'],
          '#format' => array(
            'show_header' => FALSE,
          ),
        );
      }
    }
  }
  return $result;
}

function _cck_text_diff_values(&$node, &$field) {
  $result = array();
  foreach ($node->$field['field_name'] as $item) {
    foreach (explode("\n", $item['value']) as $i) {
      $result[] = $i;
    }
  }
  return $result;
}

function _cck_userreference_diff_values($node, $field) {
  static $titles = array();
  // Gather ids.
  $ids = array();
  foreach ($node->$field['field_name'] as $item) {
    if ($item['uid'] && is_numeric($item['uid'])) {
      $ids[] = $item['uid'];
    }
  }
  // Fetch titles we don't know yet.
  $queried_ids = array_diff($ids, array_keys($titles));
  if ($queried_ids) {
    $result = db_query('SELECT uid, name FROM {users} WHERE uid IN ('. db_placeholders($queried_ids) .')', $queried_ids);
    while ($row = db_fetch_array($result)) {
      $titles[$row['uid']] = $row['name'];
    }
  }
  // Return result.
  $return = array();
  foreach ($node->$field['field_name'] as $item) {
    if ($item['uid'] && isset($titles[$item['uid']])) {
      $return[] = $titles[$item['uid']];
    }
  }
  return $return;
}

function _cck_nodereference_diff_values($node, $field) {
  static $titles = array();
  // Gather ids.
  $ids = array();
  foreach ($node->$field['field_name'] as $item) {
    if ($item['nid'] && is_numeric($item['nid'])) {
      $ids[] = $item['nid'];
    }
  }
  // Fetch titles we don't know yet.
  $queried_ids = array_diff($ids, array_keys($titles));
  if ($queried_ids) {
    dsm($queried_ids);
    $result = db_query('SELECT nid, title FROM {node} WHERE nid IN ('. db_placeholders($queried_ids) .')', $queried_ids);
    while ($row = db_fetch_array($result)) {
      $titles[$row['nid']] = $row['title'];
    }
  }
  // Return result.
  $return = array();
  foreach ($node->$field['field_name'] as $item) {
    if ($item['nid'] && isset($titles[$item['nid']])) {
      $return[] = $titles[$item['nid']];
    }
  }
  return $return;
}
