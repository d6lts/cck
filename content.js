// $Id$

Drupal.behaviors.cckManageFields = function(context) {
  attachUpdateSelects(context);
};

function attachUpdateSelects(context) {
  var widgetTypes = Drupal.settings.contentWidgetTypes;
  var fields = Drupal.settings.contentFields;

  // Store the default text of widget selects.
  $('#content-field-overview .content-widget-type-select', context).each(function() {
    this.initialValue = this.options[0].text;
  });

  // Field type select updates its widget select.
  $('#content-field-overview .content-field-type-select', context).each(function() {
    this.targetSelect = $('.content-widget-type-select', $(this).parents('tr'));

    $(this).change(function() {
      var selectedFieldType = this.options[this.selectedIndex].value;
      var options = (selectedFieldType in widgetTypes) ? widgetTypes[selectedFieldType] : [ ];
      this.targetSelect.contentOptions(options);
    });

    $(this).trigger('change');
  });

  // Existing field select updates its widget select and label textfield.
  $('#content-field-overview .content-field-select', context).each(function() {
    this.targetSelect = $('.content-widget-type-select', $(this).parents('tr'));
    this.targetTextfield = $('.content-label-textfield', $(this).parents('tr'));

    $(this).change(function(e, updateText) {
      var updateText = (typeof(updateText) == 'undefined') ? true : updateText;
      var selectedField = this.options[this.selectedIndex].value;
      var selectedFieldType = (selectedField in fields) ? fields[selectedField].type : null;
      var selectedFieldWidget = (selectedField in fields) ? fields[selectedField].widget : null
      var options = (selectedFieldType && (selectedFieldType in widgetTypes)) ? widgetTypes[selectedFieldType] : [ ];
      this.targetSelect.contentOptions(options, selectedFieldWidget);

      if (updateText) {
        $(this.targetTextfield)
          .attr('value', (selectedField in fields) ? fields[selectedField].label : '');
      }
    });

    $(this).trigger('change', false);
  });
}

jQuery.fn.contentOptions = function(options, selected) {
  return this.each(function() {
    var disabled = false;
    if (options.length == 0) {
      options = [this.initialValue];
      disabled = true;
    }

    var selectValue = selected || this.options[this.selectedIndex].value;

    var html = '';
    jQuery.each(options, function(value, text) {
      selected = (value == selectValue) ? ' selected="selected"' : '';
      html += '<option value="' + value + '"' + selected +'>' + text + '</option>';
    });

    $(this)
      .html(html)
      .attr('disabled', disabled ? 'disabled' : '');
  });
}