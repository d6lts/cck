// $Id$

IMPORTANT : this is experimental Drupal 6 code, not ready for any kind of use.
Use 4.7 or 5 branch releases instead !

Content Construction Kit
------------------------
To install, place the entire CCK folder into your modules directory.
Go to Administer -> Site building -> Modules and enable the Content module and one or
more field type modules:

- text.module
- number.module
- userreference.module
- nodereference.module

Now go to Administer -> Content management -> Content types. Create a new
content type and edit it to add some fields. Then test by creating
a new node of your new type using the Create content menu link.

The included optionswidget.module provides radio and check box selectors
for text and numeric types.

The included fieldgroup.module allows you to group fields together
in fieldsets to help organize them.

A comprehensive guide to using CCK is available as a CCK Handbook
at http://drupal.org/node/101723.

jvandyk [at] iastate.edu
jchaffer [at] structureinteractive.com
