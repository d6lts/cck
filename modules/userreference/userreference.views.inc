<?php
// $Id$

/**
 * @file
 * Interface between userreference.module and views.module.
 */
function userreference_views_plugins() {
  $path = drupal_get_path('module', 'userreference');
  $plugins = array(
    'module' => 'userreference', // This just tells our themes are elsewhere.
    'display' => array(
      'userreference_simple' => array(
        // Those strings are not translated for now.
        // We'll need to change that if / when we remove 'no ui'
        'title' => 'Simple', // TODO: better name ? (currently not displayed anyway)
        'help' => 'Destination-agnostic display. Mostly useful for programmatic views.',
        'no ui' => TRUE, // Programmatic use only.
        'handler' => 'userreference_plugin_display_simple',
        'uses hook menu' => FALSE,
        'use ajax' => FALSE,
        'use pager' => FALSE,
        'accept attachments' => FALSE,
      ),
    ),
    'style' => array(
      'userreference_php_array' => array(
        // Those strings are not translated for now.
        // We'll need to change that if / when we remove 'no ui'
        'title' => 'Results array (with title)',
        'help' => 'Returns the view as a PHP array of names + rendered rows.',
        'handler' => 'userreference_plugin_style_php_array',
        'no ui' => TRUE, // Programmatic use only.
        'uses row plugin' => TRUE,
        'uses fields' => TRUE,
        'type' => 'userref',
        'even empty' => TRUE,
      ),
    ),
  );
  return $plugins;
}

/**
 * Display plugin
 */
class userreference_plugin_display_simple extends views_plugin_display {
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
class userreference_plugin_style_php_array extends views_plugin_style {
  function render() {
    $results = array();

    // Group the rows according to the grouping field, if specified.
    $sets = $this->render_grouping($this->view->result, $this->options['grouping']);

    $base_field = $this->view->base_field;
    $title_field = $this->view->field['name']->field_alias;

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

/**
 * Implementation of hook_views_data_alter().
 * Adds a string filter handler to the users/name field for use with autocomplete.
 */
function userreference_views_data_alter(&$data) {
  $data['users']['name']['filter'] = array('handler' => 'views_handler_filter_string');
}

