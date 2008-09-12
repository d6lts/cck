<?php
// $Id$
?>
<div>
  <?php print $help; ?>
</div>
<?php if ($rows): ?>
  <table id="content-display-overview" class="sticky-enabled">
    <thead>
      <tr>
        <th><?php print t('Field'); ?></th>
        <?php if ($simple): ?>
          <th><?php print t('Label'); ?></th>
        <?php endif; ?>
        <?php foreach ($contexts as $context => $title): ?>
          <th><?php print $title; ?></th>
        <?php endforeach; ?>
      </tr>
    </thead>
    <tbody>
      <?php
      $count = 0;
      foreach ($rows as $row): ?>
        <tr class="<?php print $count % 2 == 0 ? 'odd' : 'even'; ?>">
          <td><?php print $row->indentation; ?><span class="<?php print $row->label_class; ?>"><?php print $row->human_name; ?></span></td>
          <?php if ($simple): ?>
            <td><?php print $row->label; ?></td>
          <?php endif; ?>
          <?php foreach ($contexts as $context => $title): ?>
            <td><?php print $row->{$context}; ?></td>
          <?php endforeach; ?>
        </tr>
        <?php $count++;
      endforeach; ?>
    </tbody>
  </table>
  <?php print $submit; ?>
<?php endif; ?>
