// $Id$

Theming instructions
====================

Template file
-------------
In order to customize field themeing :

- copy the 'content-field.tpl.php' template file into your theme's root folder
 (please keep the contents of the 'cck/theme' folder untouched. For the same
 reason, it is advised to *copy* the file instead of just moving it).

- edit that copy to your liking.


Template suggestions
--------------------
In addition, the theme layer will also look for field-specific variants (suggestions),
in the following order of precedence :

- content-field-<FIELD_NAME>-<CONTENT_TYPE_NAME>.tpl.php
  (ex : content-field-field_myfield-story.tpl.php)
  If present, will be used to theme the 'field_myfield' field when displaying a
  'story' node.

- content-field-<CONTENT_TYPE_NAME>.tpl.php
  (ex : content-field-story.tpl.php)
  If present, will be used to theme all fields of 'story' nodes.

- content-field-<FIELD_NAME>.tpl.php
  (ex : content-field-field_myfield.tpl.php)
  If present, will be used to theme all 'field_myfield' field in all the content
  types it appears in.

- content-field.tpl.php
  If none of the above is present, the base template will be used.

IMPORTANT :
Suggestions work only if the theme also has the base template.
If your theme has content-field-*.tpl.php files, it must also have a
content-field.tpl.php file.


Formatters theme function
-------------------------
In CCK 2.0 for Drupal 6, all formatters now go through the theme layer.

Overriding a formatter's theme is another way you can alter how your values
are displayed (whereas changing content-field.tpl.php lets you change the HTML
that "wraps" the values.

This can be done using the regular D6 theming practices (see http://drupal.org/theme-guide).




