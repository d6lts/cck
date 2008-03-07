<?php
// $Id$

// Two helper functions that generate appropriate tokens for CCK-added fields.
function content_token_values($type, $object = NULL) {
  $tokens = array();
  if ($type == 'node') {
    $node = $object;
    content_view($node);

    $field_types = _content_field_types();

    foreach (content_fields() as $field) {

      $items = $node->$field['field_name'];
      if (!empty($items)) {
        $function = $field_types[$field['type']]['module'] . '_token_values';
        if (function_exists($function)) {
          $sub_list = $function('field', $items);
          foreach($sub_list as $token => $value) {
            $tokens[$field['field_name'] .'-'. $token] = $value;
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

if (module_exists('nodereference')) {
  function nodereference_token_list($type = 'all') {
    if ($type == 'field' || $type == 'all') {
      $tokens = array();

      $tokens['node reference']['nid']   = t("Referenced node ID.");
      $tokens['node reference']['title'] = t("Referenced node title.");
      $tokens['node reference']['link']  = t("Formatted html link to the node.");

      return $tokens;
    }
  }

  function nodereference_token_values($type, $object = NULL) {
    if ($type == 'field') {
      $item = $object[0];

      $tokens['nid']   = $item['nid'];
      $tokens['title'] = strip_tags($item['view']);
      $tokens['link']  = $item['view'];

      return $tokens;
    }
  }
}

if (module_exists('number')) {
  function number_token_list($type = 'all') {
    if ($type == 'field' || $type == 'all') {
      $tokens = array();

      $tokens['number']['raw']       = t("Raw number value.");
      $tokens['number']['formatted'] = t("Formatted number value.");

      return $tokens;
    }
  }

  function number_token_values($type, $object = NULL) {
    if ($type == 'field') {
      $item = $object[0];

      $tokens['raw']       = $item['value'];
      $tokens['formatted'] = $item['view'];

      return $tokens;
    }
  }
}

if (module_exists('text')) {
  function text_token_list($type = 'all') {
    if ($type == 'field' || $type == 'all') {
      $tokens = array();

      $tokens['text']['raw']       = t("Raw, unfiltered text.");
      $tokens['text']['formatted'] = t("Formatted and filtered text.");

      return $tokens;
    }
  }

  function text_token_values($type, $object = NULL) {
    if ($type == 'field') {
      $item = $object[0];
      $tokens['raw']  = $item['value'];
      $tokens['formatted'] = $item['view'];
      return $tokens;
    }
  }
}

if (module_exists('userreference')) {
  function userreference_token_list($type = 'all') {
    if ($type == 'field' || $type == 'all') {
      $tokens = array();

      $tokens['user reference']['uid']   = t("Referenced user ID");
      $tokens['user reference']['name']  = t("Referenced user name");
      $tokens['user reference']['link']  = t("Formatted html link to referenced user");

      return $tokens;
    }
  }

  function userreference_token_values($type, $object = NULL) {
    if ($type == 'field') {
      $item = $object[0];

      $tokens['uid']   = $item['uid'];
      $tokens['name']  = strip_tags($item['view']);
      $tokens['link']  = $item['view'];

      return $tokens;
    }
  }
}

if (module_exists('link')) {
  function link_token_list($type = 'all') {
    if ($type == 'field' || $type == 'all') {
      $tokens = array();

      $tokens['link']['url']    = t("Link URL");
      $tokens['link']['title']  = t("Link title");
      $tokens['link']['view']   = t("Formatted html link");

      return $tokens;
    }
  }

  function link_token_values($type, $object = NULL) {
    if ($type == 'field') {
      $item = $object[0];

      $tokens['url']    = $item['url'];
      $tokens['title']  = $item['title'];
      $tokens['view']   = $item['view'];

      return $tokens;
    }
  }
}

if (module_exists('imagefield')) {
  function imagefield_token_list($type = 'all') {
    if ($type == 'field' || $type == 'all') {
      $tokens = array();

      $tokens['image']['fid']       = t("Image file ID");
      $tokens['image']['title']     = t("Image title");
      $tokens['image']['alt']       = t("Image alt text");
      $tokens['image']['filename']  = t("Image file name");
      $tokens['image']['filepath']  = t("Image path");
      $tokens['image']['filemime']  = t("Image MIME type");
      $tokens['image']['filesize']  = t("Image file size");
      $tokens['image']['view']      = t("Fully formatted HTML image tag");

      return $tokens;
    }
  }

  function imagefield_token_values($type, $object = NULL) {
    if ($type == 'field') {
      $item = $object[0];

      $tokens['fid']      = $item['fid'];
      $tokens['title']    = $item['title'];
      $tokens['alt']      = $item['alt'];
      $tokens['filename'] = $item['filename'];
      $tokens['filepath'] = $item['filepath'];
      $tokens['filemime'] = $item['filemime'];
      $tokens['filesize'] = $item['filesize'];
      $tokens['view']     = $item['view'];

      return $tokens;
    }
  }
}