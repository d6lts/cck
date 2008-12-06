<?php
// $Id$

/**
 * Implementation of hook_nodeapi_load.
 */
function content_nodeapi_load($nodes, $types) {
  $queried_nodes = array();
  // Fetch avaliable nodes from cache.
  foreach ($nodes as $nid => $node) {
    $cid = 'content:'. $node->nid .':'. $node->vid;
    if ($cached = cache_get($cid, content_cache_tablename())) {
      foreach ($cached->data as $key => $value) {
        $node->$key = $value;
      }
    }
    else {
      $queried_nodes[$nid] = $nodes[$nid];
    }
  }
  // Fetch other nodes from the database.
  if ($queried_nodes) {
    // We need the raw additions to be able to cache them, so content_storage_load()
    // and hook_field_load() should not alter nodes directly but return their
    // additions.
    // TODO D7 : revise that if/when http://drupal.org/node/111127 (Cache node_load) gets in ?
    // We'd still need a cache for 'fields on non-nodes...'.
    $additions = content_storage_load($queried_nodes);
    foreach ($additions as $nid => $node_additions) {
      foreach ($node_additions as $key => $value) {
        $queried_nodes[$nid]->$key = $value;
      }
    }
    // TODO D7 : to be consistent we need to make hook_field_load() accept
    // multiple nodes too. Requires de-opifying hook_field...
    //$custom_additions = _content_field_invoke('load', $queried_nodes);
    $custom_additions = array();
    foreach ($custom_additions as $nid => $node_additions) {
      foreach ($node_additions as $key => $value) {
        $queried_nodes[$nid]->$key = $value;
        $additions[$nid][$key] = $value;
      }
    }
    // Cache the data.
    foreach ($queried_nodes as $nid => $node) {
      $cid = 'content:'. $node->nid .':'. $node->vid;
      $data =  isset($additions[$nid]) ? $additions[$nid] : array();
      cache_set($cid, $data, content_cache_tablename());
    }
  }
}

/**
 * Implementation of hook_nodeapi_validate.
 *
 */
function content_nodeapi_validate(&$node, $form = NULL) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      _content_field_invoke('validate', $node, $form);
      _content_field_invoke_default('validate', $node, $form);
    }
  }
}

/**
 * Implementation of hook_nodeapi_presave.
 */
function content_nodeapi_presave(&$node) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      _content_field_invoke('presave', $node);
      _content_field_invoke_default('presave', $node);
    }
  }
}

/**
 * Implementation of hook_nodeapi_insert.
 */
function content_nodeapi_insert(&$node) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      content_storage_write($node);
      _content_field_invoke_default('insert', $node);
    }
  }
}

/**
 * Implementation of hook_nodeapi_update.
 */
function content_nodeapi_update(&$node) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      content_storage_write($node, TRUE);
      _content_field_invoke_default('update', $node);
      cache_clear_all('content:'. $node->nid .':'. $node->vid, 'cache_content');
    }
  }
}

/**
 * Implementation of hook_nodeapi_delete.
 */
function content_nodeapi_delete(&$node) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      content_storage_delete($node);
      _content_field_invoke_default('delete', $node);
      cache_clear_all('content:'. $node->nid .':', 'cache_content', TRUE);
    }
  }
}

/**
 * Implementation of hook_nodeapi_delete_revision.
 */
function content_nodeapi_delete_revision(&$node) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      content_storage_delete_revision($node);
      _content_field_invoke_default('delete revision', $node);
      cache_clear_all('content:'. $node->nid .':'. $node->vid, 'cache_content');
    }
  }
}

/**
 * Implementataion of content_nodeapi_view.
 */
function content_nodeapi_view(&$node, $teaser = FALSE, $page = FALSE) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      // Let field modules sanitize their data for output.
      // TODO D7 : we could reintegrate hook_field('view') for this ?
      _content_field_invoke('sanitize', $node, $teaser, $page);

      // Merge fields.
      $additions = _content_field_invoke_default('view', $node, $teaser, $page);
      $node->content = array_merge((array) $node->content, $additions);
    }
  }
}

/**
 * Implementation of hook_nodeapi_alter.
 *
 * Add back the formatted values in the 'view' element for all fields,
 * so that node templates can use it.
 */
function content_nodeapi_alter(&$node, $teaser = FALSE, $page = FALSE) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      _content_field_invoke_default('alter', $node, $teaser, $page);
    }
  }
}

/**
 * Implementation of hook_nodeapi_prepare_translation.
 */
function content_nodeapi_prepare_translation(&$node) {
  // Prevent against invalid 'nodes' built by broken 3rd party code.
  if (isset($node->type)) {
    $type = content_types($node->type);
    // Save cycles if the type has no fields.
    if (!empty($type['instances'])) {
      $default_additions = _content_field_invoke_default('prepare translation', $node);
      $additions = _content_field_invoke('prepare translation', $node);
      // Merge module additions after the default ones to enable overriding
      // of field values.
      $node = (object) array_merge((array) $node, $default_additions, $additions);
    }
  }
}
