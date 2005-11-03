Content Construction Kit
------------------------
For information about the CCK, see http://drupal.org/cck-status

This is still in development. Do not use on production sites.

To test, add cck.mysql to your database. Place the cck folder
into your modules directory. Go to administer -> modules and enable
the content module and one or more field type modules:
- text.module
- number.module
- date.module

Now go to administer -> content -> content types. Create a new
content type and edit it to add some fields. Then test by
creating a new node of your new type.

jvandyk [at] iastate.edu
jchaffer [at] structureinteractive.com