<?php

function phptemplate_field(&$node, &$field, &$items, $teaser, $page) {
  $variables = array(
    'node' => $node,
    'field' => $field,
    'field_type' => $field['type'],
    'label' => $field['widget']['label'],
    'items' => $items,
    'teaser' => $teaser,
    'page' => $page,
  );

  return _phptemplate_callback('field', $variables, 'field-'. $field['field_name']);
}