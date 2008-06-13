<?php
// $Id$

/**
 * Create fields' form for a content type.
 *
 * Each field defines its own component of the content entry form, via its
 * chosen widget.
 */
function content_form(&$form, &$form_state) {
  $type = content_types($form['type']['#value']);
  foreach ($type['fields'] as $field_name => $field) {
    $form['#field_info'][$field['field_name']] = $field;
    $form += (array) content_field_form($form, $form_state, $field);
  }
  return $form;
}

/**
 * Create a separate form element for each field.
 *
 * // TODO : $count param ? not used anymore ?
 * Hook_widget() picks up two new values, $count and $delta, to help
 * widgets know what information to return since multiple values are
 * sometimes controlled by the content module.
 *
 * @param $form
 *   the form to add this field element to
 * @param $form_state
 *   the form_state for the above form
 * @param $field
 *   the field array to use to create the form element
 * @param $get_delta
 *   use to get only a specific delta value of a multiple value field, otherwise
 *   function will return the entire $field form element.
 */
function content_field_form(&$form, &$form_state, $field, $get_delta = NULL) {
  $form['#cache'] = FALSE;
  $node = $form['#node'];
  $addition = array();
  $form_element = array();
  // TODO : is the "if (function_exists($function)) {" needed ?
  // defining the $function here makes it unclear where it is actually called
  $function = $field['widget']['module'] .'_widget';
  $field_name = $field['field_name'];
  if (function_exists($function)) {
    // Prepare the values to be filled in the widget.
    // We look in the following places :
    // - Form submitted values
    // - Node values (when editing an existing node), or pre-filled values (when
    //   creating a new node translation)
    // - Default values set for the field (when creating a new node).
    $items = array();
    if (!empty($form_state['values'][$field['field_name']])) {
      $items = $form_state['values'][$field['field_name']];
    }
    elseif (!empty($node->$field['field_name'])) {
      $items = $node->$field['field_name'];
    }
    elseif (empty($node->nid)) {
      if (content_callback('widget', 'default value', $field) != CONTENT_CALLBACK_NONE) {
        // If a module wants to insert custom default values here,
        // it should provide a hook_default_value() function to call,
        // otherwise the content module's content_default_value() function
        // will be used.
        $callback = content_callback('widget', 'default value', $field) == CONTENT_CALLBACK_CUSTOM ? $field['widget']['module'] .'_default_value' : 'content_default_value';
        if (function_exists($callback)) {
          $items = $callback($form, $form_state, $field, 0);
        }
      }
    }

    // If content module handles multiple values for this form element,
    // and not selecting an individual $delta, process the multiple value form.
    if (!isset($get_delta) && content_handle('widget', 'multiple values', $field) == CONTENT_HANDLE_CORE) {
      $form_element = content_multiple_value_form($form, $form_state, $field, $items);
    }
    // If the widget is handling multiple values (e.g optionwidgets),
    // or selecting an individual element, just get a single form
    // element and make it the $delta value.
    else {
      $delta = isset($get_delta) ? $get_delta : 0;
      if ($element = $function($form, $form_state, $field, $items, $delta)) {
        $defaults = array(
          '#field_name' => $field['field_name'],
          '#required' => $get_delta > 0 ? FALSE : $field['required'],
          // TODO : should we add some logic for title and description too ?
          '#columns'  => array_keys($field['columns']),
          '#title' => t($field['widget']['label']),
          '#description' => t($field['widget']['description']),
          '#delta' => $delta,
        );
        // If we're processing a specific delta value for a field where the
        // content module handles multiples, set the delta in the result.
        // For fields that handle their own processing, we can't make assumptions
        // about how the field is structured, just merge in the returned value.
        if (content_handle('widget', 'multiple values', $field) == CONTENT_HANDLE_CORE) {
          $form_element[$delta] = array_merge($element, $defaults);
        }
        else {
          $form_element = array_merge($element, $defaults);
        }
      }
    }

    // Field name is needed at top level as well as the individual elements
    // so the multiple values or other field level theme or processing can find it.
    if ($form_element) {
      $defaults = array(
        '#field_name' => $field['field_name'],
        '#tree' => TRUE,
        '#weight' => $field['widget']['weight'],
        // TODO : what's the need for #count ? does not seem to be used anywhere ?
        '#count' => count($form_element),
      );
      $addition[$field['field_name']] = array_merge($form_element, $defaults);
    }
  }
  return $addition;
}

/**
 * Special handling to create form elements for multiple values.
 *
 * Handles generic features for multiple fields :
 * - number of widgets
 * - AHAH-'add more' button
 * - drag-n-drop value reordering
 */
function content_multiple_value_form(&$form, &$form_state, $field, $items) {
  $field_name = $field['field_name'];

  switch ($field['multiple']) {
    case 0:
      $max = 0;
      break;
    case 1:
      $max = (isset($form_state['item_count'][$field_name]) ? $form_state['item_count'][$field_name] : count($items));
      break;
    default:
      $max = $field['multiple'] - 1;
      break;
  }

  $form_element = array(
    // TODO : Not sure about that one - when theming through '#type',
    // children are already rendered, and we can't do the table layout.
    //'#type' => 'content_multiple_values',
    '#theme' => 'content_multiple_values',
    '#title' => t($field['widget']['label']),
    '#required' => $field['required'],
    '#description' => t($field['widget']['description']),
  );
  $function = $field['widget']['module'] .'_widget';

  for ($delta = 0; $delta <= $max; $delta++) {
    if ($element = $function($form, $form_state, $field, $items, $delta)) {
      $defaults = array(
        '#field_name' => $field_name,
        '#title' => ($field['multiple'] >= 1) ? '' : t($field['widget']['label']),
        '#description' => ($field['multiple'] >= 1) ? '' : t($field['widget']['description']),
        '#required' => $delta == 0 && $field['required'],
        '#weight' => $delta,
        '#delta' => $delta,
        '#columns' => array_keys($field['columns']),
      );

      // Add an input field for the delta (drag-n-drop reordering), which will
      // be hidden by tabledrag js behavior.
      if ($field['multiple'] >= 1) {
        // We name the element '_weight' to avoid clashing with column names
        // defined by field modules.
        $element['_weight'] = array(
          '#type' => 'weight',
          '#delta' => $max, // this 'delta' is the 'weight' element's property
          '#default_value' => $delta,
          '#weight' => 100,
        );
      }

      $form_element[$delta] = array_merge($element, $defaults);
    }
  }

  // Add AHAH add more button, if not working with a programmed form.
  if ($field['multiple'] == 1 && empty($form['#programmed'])) {
    // Make sure the form is cached so ahah can work.
    $form['#cache'] = TRUE;
    $content_type = content_types($form['type']['#value']);
    $field_name_css = str_replace('_', '-', $field_name);

    $form_element[$field_name .'_add_more'] = array(
      '#type' => 'submit',
      '#value' => t('Add another item'),
      '#weight' => $field['widget']['weight'] + $max + 1,
      '#submit' => array('content_add_more_submit'), // If no JavaScript action.
      '#ahah' => array(
        'path' => 'content/js_add_more/'. $content_type['url_str'] .'/'. $field_name,
        'wrapper' => $field_name_css .'-items',
        'method' => 'replace',
        'effect' => 'fade',
      ),
      // When JS is disabled, the content_add_more_submit handler will find
      // the relevant field using these entries.
      //'#field_name' => $field_name,
      //'#type_name' => $content_type['type'],
    );

    // Add wrappers for the fields and 'more' button.
    // TODO : could be simplified ?
    $form_element['#prefix'] = '<div class="clear-block" id="'. $field_name_css .'-add-more-wrapper"><div id="'. $field_name_css .'-items">';
    $form_element[$field_name .'_add_more']['#prefix'] = '<div class="content-add-more">';
    $form_element[$field_name .'_add_more']['#suffix'] =  '</div></div></div>';
  }
  return $form_element;
}

/**
 * Submit handler to add more choices to a content form. This handler is used when
 * JavaScript is not available. It makes changes to the form state and the
 * entire form is rebuilt during the page reload.
 */
function content_add_more_submit($form, &$form_state) {
  // Set the form to rebuild and run submit handlers.
  node_form_submit_build_node($form, $form_state);
  $field_name = $form_state['clicked_button']['#field_name'];
  $type_name = $form_state['clicked_button']['#type_name'];

  foreach ($form['#field_info'] as $field_name => $field) {
    // Make the changes we want to the form state.
    if ($form_state['values'][$field_name][$field_name .'_add_more']) {
      $form_state['item_count'][$field_name] = count($form_state['node'][$field_name]) + 1;
    }
  }
}


/**
 * Menu callback for AHAH addition of new empty widgets.
 */
function content_add_more_js($type_name_url, $field_name) {
  $type = content_types($type_name_url);
  $field = content_fields($field_name, $type['type']);
  if ($field['multiple'] != 1) {
    // TODO : is that correct ?
    drupal_json(array('status' => 0));
  }

  // Reorder values to account for drag-n-drop reordering.
  $_POST[$field_name] = _content_sort_items($field, $_POST[$field_name]);
  $delta = max(array_keys($_POST[$field_name])) + 1;
  // Retrieve the cached form.
  $form_state = array('values' => $_POST);
  $form = form_get_cache($_POST['form_build_id'], $form_state);

  // Build our new form element for the whole field,
  // and let other modules alter it.
  $form_element = content_field_form($form, $form_state, $field);
  drupal_alter('form', $form_element, array(), 'content_add_more_js');

  // Add the new element at the right place in the form.
  if (module_exists('fieldgroup') && ($group_name = _fieldgroup_field_get_group($type['type'], $field_name))) {
    $form[$group_name][$field_name] = $form_element[$field_name];
  }
  else {
    $form[$field_name] = $form_element[$field_name];
  }

  // Save the new definition of the form.
  form_set_cache($_POST['form_build_id'], $form, $form_state);

  // Build the new form so that we can render the new element.
  $_POST[$field_name][$delta]['_weight'] = $delta;   // TODO : Hack !!
  $form_state = array('submitted' => FALSE);
  $form += array(
    '#post' => $_POST,
    '#programmed' => FALSE,
  );
  $form = form_builder($_POST['form_id'], $form, $form_state);

  // Render the new output.
  $field_form = (!empty($group_name)) ? $form[$group_name][$field_name] : $form[$field_name];
  // We add a div around the new content to receive the ahah effect.
  $field_form[$delta]['#prefix'] = '<div class="ahah-new-content">'. (isset($field_form[$delta]['#prefix']) ? $field_form[$delta]['#prefix'] : '');
  $field_form[$delta]['#suffix'] = (isset($field_form[$delta]['#suffix']) ? $field_form[$delta]['#suffix'] : '') .'</div>';
  $output = theme('status_messages') . drupal_render($field_form);
  drupal_json(array('status' => TRUE, 'data' => $output));
}
