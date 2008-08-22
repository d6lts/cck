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
        // Those strings are not translated for now.
        // We'll need to change that if / when we remove 'no ui'
        'title' => 'Simple', // TODO: better name ? (currently not displayed anyway)
        'help' => 'Destination-agnostic display. Mostly useful for programmatic views.',
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
        // Those strings are not translated for now.
        // We'll need to change that if / when we remove 'no ui'
        'title' => 'Results array (with title)',
        'help' => 'Returns the view as a PHP array of titles + rendered rows.',
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
}
