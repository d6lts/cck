<?php
// $Id$

/**
 * @file
 * Interface between nodereference.module and views.module.
 */
function nodereference_views_plugins() {
  $path = drupal_get_path('module', 'nodereference');
  $plugins = array(
    'module' => 'nodereference', // This just tells our themes are elsewhere.
    'display' => array(
      'nodereference_simple' => array(
        'title' => t('Simple'), // TODO: better name ? (currently not displayed anyway)
        'help' => t('Destination-agnostic display. Mostly useful for programmatic views.'),
        'no ui' => TRUE, // Programmatic use only.
        'handler' => 'nodereference_plugin_display_simple',
        'uses hook menu' => FALSE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
      ),
    ),
    'style' => array(
      'nodereference_php_array' => array(
        'title' => t('Results array'),
        'help' => t('Returns the view as a PHP array of titles + renderd rows.'),
        'handler' => 'nodereference_plugin_style_php_array',
        'no ui' => TRUE, // Programmatic use only.
        'uses row plugin' => TRUE,
        'uses fields' => TRUE,
        'type' => 'noderef',
        'even empty' => TRUE,
      ),
    ),
  );
  return $plugins;
}

/**
 * Display plugin
 */
class nodereference_plugin_display_simple extends views_plugin_display {
  function execute() {
    return $this->view->render($this->display->id);
  }

  function render() {
    return !empty($this->view->result) || !empty($this->view->style_plugin->definition['even empty']) ? $this->view->style_plugin->render($this->view->result) : '';
  }
}

/**
 * Style plugin that simply return a PHP array of themed rows.
 */
class nodereference_plugin_style_php_array extends views_plugin_style {
  function render() {
    $results = array();

    // Group the rows according to the grouping field, if specified.
    $sets = $this->render_grouping($this->view->result, $this->options['grouping']);

    $base_field = $this->view->base_field;
    $title_field = $this->view->field['title']->field_alias;

    // TODO : We don't display grouping info for now.
    // Could be useful for select widget, though.
    foreach ($sets as $title => $records) {
      foreach ($records as $label => $row) {
        $results[$row->{$base_field}] = array(
          'title' => $row->{$title_field},
          'rendered' => $this->row_plugin->render($row),
        );
      }
    }
    return $results;
  }

//  function render() {
//    $results = array();
//    if (empty($this->row_plugin)) {
//      vpr('nodereference_plugin_style_results_array: Missing row plugin');
//      return $results;
//    }
//
//    // Group the rows according to the grouping field, if specified.
//    $sets = $this->render_grouping($this->view->result, $this->options['grouping']);
//
//    // Render each group separately.
//    $base_field = $this->view->base_field;
//    foreach ($sets as $title => $records) {
//      $rows = array();
//      foreach ($records as $label => $row) {
//        $rows[$row->{$base_field}] = $this->row_plugin->render($row);
//      }
//      $results[$title] = $rows;
//    }
//    return $results;
//  }
}
