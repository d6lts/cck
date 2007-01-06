// $Id$

Theming Constructed Content Types

After you have designed your content, you will likely want to lay out the
presentation of the content. This is most easily done with PHPtemplate-based
themes. There are two basic options for theming your content, depending on the
level of control you need to exert.


Theming individual fields

This method is straightforward and allows for sensible defaults, but requires a
modest amount of setup. Place the "field.tpl.php" and "template.php" files in
your theme's folder. If you already have a "template.php" file, you will need
to append the code in this one to your own.

All fields are now rendered using your "field.tpl.php" template. If you want to
change them all the same way (for example, to hide every field label), you can
make alterations to this file. If you want to change the presentation of one
field independently, you should instead make a copy of this file first, and
give it the name "field-field_foo.tpl.php" where "field_foo" is the field name
as listed on the administration page. Changes you make to this file will be
reflected only in that field.

An $items variable is made available to you in your field template. This
contains the content of the field you are rendering. It is always an array, so
that the syntax is consistent whether or not the field allows for multiple
values. Each item has a "view" property that contains the filtered, formatted
contents of the item. You should always use this property in your display for
security reasons, unless you are very familiar with how to properly process
output and avoid scripting exploits.

The included example here ("field-field_body.tpl.php") illustrates a
minimalistic approach to field theming, for a single-valued field. For a more
flexible template, start from the included "field.tpl.php".


Theming the node as a whole

If you need more flexibility than is afforded by theming individual fields, you
may theme the entire node as a unit. This allows you to affect field order, to
change the HTML structure to something more complicated, like a table, or even
to exclude fields from the presentation entirely. The setup for theming a node
is simpler than for theming a field; simply create a file called
"node-content_foo.tpl.php" where "content_foo" is the content type name as
listed on the administration page. For an example of the typical contents of
this file, investigate the included example here
("node-content_example.tpl.php") or the "node.tpl.php" file that comes with
your theme.
