<?php

function phptemplate_field(&$node, &$field, &$items, $teaser, $page) {
  $variables = array(
    'node' => $node,
    'field' => $field,
    'field_type' => $field['type'],
    'field_name' => $field['field_name'],
    'label' => $field['widget']['label'],
    'items' => $items,
    'teaser' => $teaser,
    'page' => $page,
  );

  return _phptemplate_callback('field', $variables, array('field-'. $field['field_name']));
}