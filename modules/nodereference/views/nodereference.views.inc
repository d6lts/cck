<?php
// $Id$

/**
 * @file
 * Views integration interface for nodereference module.
 */

/**
 * Implementation of hook_views_handlers().
 */
function nodereference_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'nodereference') . '/views/handlers',
    ),
    'handlers' => array(
      'nodereference_handler_relationship_reverse' => array(
        'parent' => 'views_handler_relationship',
      ),
    ),
  );
}
