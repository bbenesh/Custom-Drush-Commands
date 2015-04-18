<?php

/**
 * @file
 * Custom drush commands for blog functions.
 *
 * AKA things I might have to do repeatedly that are hard to do in the UI.
 */

/**
 * Implements hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and
 * description.
 *
 * @return array
 *   An associative array describing your command(s).
 */
function blog_custom_drush_command() {
  $items = array();

  // The command.
  $items['blog-update-author'] = array(
    'description' => "Remove a blog author and replace it with another",
    'callback' => 'drush_blog_custom_update_author',
    'arguments' => array(
      'old_author' => 'The nid of the author to remove.',
      'new_author' => 'The (optional) nid of the author to replace old_author with',
    ),
  );

  return $items;
}

/**
 * Remove or replace a blog author entity reference.
 *
 * Given the nid of an author node, this function will either just remove that
 * author reference or, if also supplied with the nid of another author node,
 * will replace the old author reference with a new author reference.
 *
 * @param int $old_author
 *   NID of author node entity reference to remove.
 * @param int $new_author
 *   NID of author node to reference in place of $old_author.
 */
function drush_blog_custom_update_author($old_author, $new_author = NULL) {

  $site_name = variable_get('site_name');
  $old_author_node = node_load($old_author);

  // Check if this is a valid author node.
  if ($old_author_node) {
    // Start message for user confirmation prompt.
    $message = "Are you sure you want to update blog author reference for $site_name? This will remove $old_author_node->title";

    // If a new author has been provided, let's validate
    // that too before we get ahead of ourselves.
    if ($new_author) {
      $new_author_node = node_load($new_author);

      if ($new_author_node) {
        // If the new author is a valid node, tack this onto the user prompt.
        $message .= " and replace it with $new_author_node->title";
      }
      else {
        return drush_set_error("No author node with the nid $new_author.");
      }
    }
    $message .= " for all blog posts.";
  }
  else {
    return drush_set_error("No author node with the nid $old_author.");
  }

  // Prompt user to confirm this action.
  $user_confirm = drush_choice(array('y' => 'Yes'), $message);

  if ($user_confirm) {

    // Search for all blog posts referencing $old_author
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', 'blog_post')
      ->fieldCondition('field_author', 'target_id', $old_author_node->nid, '=')
      ->execute();

    // If we don't have results, let us know.
    if (!isset($result['node'])) {
      drush_log("No nodes to process.", "ok");
      return;
    }

    // Let's loop through each post and update it.
    foreach ($result['node'] as $nid => $stub) {

      // EntityMetadataWrapper lazy loads nodes for us and makes them
      // more like real objects, saving us weird drupal nested array value pain.
      $wrapper = entity_metadata_wrapper("node", $nid);

      // Remember to clear our our variables before the field-level loop!
      $new_author_already_exists = FALSE;
      unset($new_author_delta);
      unset($old_author_delta);

      // Let's loop through our multivalue entity reference field.
      foreach ($wrapper->field_author as $delta => $field) {

        // If we were given a $new_author...
        if ($new_author_node) {
          // Let's check to see if it's already referenced
          // and also a "just-in-case" check to handle the off-chance that
          // someone has entered a new author that's the same as old author.
          if ($field->nid->value() == $new_author_node->nid && $field->nid->value() != $old_author_node->nid) {
            $new_author_already_exists = TRUE;
            $new_author_delta = $delta;
          }
        }

        // Store the $delta of $old_author so we can unset it.
        if ($field->nid->value() == $old_author_node->nid) {
          $old_author_delta = $delta;
        }

      }

      // If there is no new author given
      // OR the new author is already referenced in the field_author array...
      if ($new_author == NULL || $new_author_already_exists == TRUE) {

        // Just bein' verbose with my output, for my own sanity.
        if ($new_author == NULL) {
          $msg = "No new author set. ";
        }
        if ($new_author_already_exists == TRUE) {
          $msg = "$new_author_node->title is already referenced in the array. ";
        }

        // Unset the old one and we're done.
        drush_log($msg . "Removing $old_author_node->title from field_author[$delta] in node $nid.", "ok");
        unset($msg);
        unset($wrapper->field_author[$old_author_delta]);
      }

      // If the new author doesnt exist in the field_author array yet...
      // or for some unknown reason is the same as the old author we
      // just eradicated...
      else {
        // Replace old author with new author.
        drush_log("Replacing $old_author_node->title in field_author[$delta] with $new_author_node->title in node $nid.", "ok");
        $wrapper->field_author[$old_author_delta] = $new_author_node;
      }

      // Save the node!
      $wrapper->save();
    }
  }
}
