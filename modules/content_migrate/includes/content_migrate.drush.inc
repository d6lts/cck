<?php
// $Id$
/**
 * @file content_migrate.admin.inc
 * Code to process field data migration, moved into a separate file for efficiency.
 */

/**
 * Implementation of hook_drush_help().
 */
function content_migrate_drush_help($section) {
  switch ($section) {
    case 'content migrate:content-migrate':
      return dt("Migrate all fields");
    case 'content migrate:content-migrate-structure':
      return dt("Migrate a single field structure");
    case 'content migrate:content-migrate-data':
      return dt("Migrate a single field data");
  }
}


/**
 * Implementation of hook_drush_command().
 */
function content_migrate_drush_command() {
  $items['content-migrate'] = array(
    'description' => 'Migrate fields.',
  );
  $items['content-migrate-field-structure'] = array(
    'description' => 'Migrate a single field structure.',
    'arguments' => array(
      'name' => 'A field system name.',
    ),
  );
  $items['content-migrate-field-data'] = array(
    'description' => 'Migrate a single field data.',
  );

  return $items;
}

/**
 * Command callback.
 */
function drush_content_migrate() {
  module_load_include('inc', 'content_migrate', 'includes/content_migrate.admin');
  $fields = content_migrate_get_options();
  $field_names = array_keys($fields['available']);

  foreach ($field_names as $field_name) {
    drush_invoke_process_args('content-migrate-field-structure', array($field_name));
  }
  foreach ($field_names as $field_name) {
    drush_invoke_process_args('content-migrate-field-data', array($field_name));
  }
}

function drush_content_migrate_field_structure($field_name) {
  drush_log('Migrating structure: ' . $field_name);
  module_load_include('inc', 'content_migrate', 'includes/content_migrate.admin');
  _content_migrate_batch_process_create_fields(array($field_name));
}

function drush_content_migrate_field_data($field_name) {
  drush_log('Migrating data: ' . $field_name);
  module_load_include('inc', 'content_migrate', 'includes/content_migrate.admin');
  $context = array(
    'sandbox' => array(),
  );
  _content_migrate_batch_process_migrate_data($field_name, $context);
}
