<?php

/**
 * @file
 * Contains the SearchApiEtDatasourceController class.
 */

/**
 * Provides multilingual versions of all entity types.
 */
class SearchApiEtDatasourceController extends SearchApiEntityDataSourceController {

  /**
   * Overrides SearchApiEntityDataSourceController::$table.
   *
   * Needed because we have a string ID, instead of a numerical one.
   *
   * @var string
   */
  protected $table = 'search_api_et_item';

  /**
   * {@inheritdoc}
   */
  public function getIdFieldInfo() {
    return array(
      'key' => 'search_api_et_id',
      'type' => 'string',
    );
  }

  /**
   * {@inheritdoc}
   */
  public function loadItems(array $ids) {
    $item_languages = array();
    foreach ($ids as $id) {
      list($langcode, $entity_id) = explode('_', $id, 2);
      $item_languages[$entity_id][] = $langcode;
    }
    $entities = entity_load($this->entityType, array_keys($item_languages));

    // If some items could not be loaded, remove them from tracking.
    if (count($entities) != count($item_languages)) {
      $unknown = array_keys(array_diff_key($item_languages, $entities));
      if ($unknown) {
        $deleted = array();
        foreach ($unknown as $entity_id) {
          foreach ($item_languages[$entity_id] as $langcode) {
            $deleted[] = "{$langcode}_{$entity_id}";
          }
        }
        search_api_track_item_delete($this->type, $deleted);
      }
    }

    // Now arrange them according to our IDs again, with language.
    $items = array();
    foreach ($item_languages as $entity_id => $langs) {
      if (!empty($entities[$entity_id])) {
        foreach ($langs as $lang) {
          $id = "{$lang}_{$entity_id}";
          $entity = clone $entities[$entity_id];
          $entity->search_api_et_id = $id;
          $entity->language = $lang;
          $items[$id] = $entity;
        }
      }
    }

    return $items;
  }

  /**
   * {@inheritdoc}
   */
  public function getMetadataWrapper($item = NULL, array $info = array()) {
    // Since this is usually called with a "property info alter" callback
    // already in place (and only one value is allowed), we have to call
    // the existing callback from within our own callback to make it work.
    $property_info_alter = isset($info['property info alter']) ? $info['property info alter'] : NULL;
    $callback = new SearchApiEtPropertyInfoAlter($property_info_alter);
    $info['property info alter'] = array($callback, 'propertyInfoAlter');

    // If the item isn't the object and a multilingual id is provided
    // extract the entity id to load and wrap the entity.
    if (!is_object($item) && is_scalar($item)) {
      list($item_language, $item) = explode('_', $item);
    }

    $wrapper = entity_metadata_wrapper($this->entityType, $item, $info);
    if (!empty($item->search_api_language)) {
      $wrapper = $wrapper->language($item->search_api_language);
    }
    return $wrapper;
  }

  /**
   * {@inheritdoc}
   */
  public function getItemId($item) {
    return isset($item->search_api_et_id) ? $item->search_api_et_id : NULL;
  }

  /**
   * Overrides SearchApiEntityDataSourceController::startTracking().
   *
   * Reverts the behavior to always use getAllItemIds(), instead of taking a
   * shortcut via "base table".
   *
   * This method will also be called when the multilingual configuration of an
   * index changes, to take care of new and/or out-dated IDs.
   */
  public function startTracking(array $indexes) {
    // Since existing items are index-specific, we have to do this one by one.
    // (Although in nearly all cases, this will be called with only a single
    // index anyways.)
    foreach ($indexes as $index) {
      // Get all existing IDs, in case we are just updating the items, not
      // starting fresh.
      $sql = 'SELECT item_id FROM {' . $this->table . '} WHERE index_id = :id';
      $existing = db_query($sql, array(':id' => $index->id));
      $existing = $existing->fetchAllKeyed(0, 0);
      // Get all IDs right now.
      $all_ids = $this->getAllIndexItemIds($index);
      // Compare the two sets in both directions to determine new and out-dated
      // items.
      if ($new_ids = array_diff_key($all_ids, $existing)) {
        $this->trackItemInsert($new_ids, array($index));
      }
      if ($old_ids = array_diff_key($existing, $all_ids)) {
        // We cannot use search_api_track_item_delete() here, since that would
        // delete the item for ALL indexes with this type, for which the
        // settings might differ. Therefore, we just extract the relevant code
        // here.
        // (We do not need to check for read_only, as this method will never be
        // called for read-only indexes anyways.)
        $this->trackItemDelete($old_ids, array($index));
        if ($index->server) {
          $server = $index->server();
          if ($server->enabled) {
            $server->deleteItems($old_ids, $index);
          }
          else {
            $tasks = variable_get('search_api_tasks', array());
            foreach ($old_ids as $id) {
              $tasks[$server->machine_name][$index->machine_name][] = 'delete-' . $id;
            }
            variable_set('search_api_tasks', $tasks);
          }
        }
      }
    }
  }

  /**
   * Retrieves all item IDs for a given index.
   *
   * Is used instead of SearchApiAbstractDataSourceController::getAllItemIds(),
   * since available items depend on the index configuration.
   *
   * @param SearchApiIndex $index
   *   The index for which item IDs should be retrieved.
   *
   * @return array
   *   An array with all item IDs for a given index, with keys and values both
   *   being the IDs.
   */
  public function getAllIndexItemIds(SearchApiIndex $index) {
    $entities = entity_load($this->entityType);
    $ids = array();

    foreach ($entities as $id => $entity) {
      foreach (search_api_et_item_languages($entity, $this->entityType, $index) as $lang) {
        $lang_id = "{$lang}_{$id}";
        $ids[$lang_id] = $lang_id;
      }
    }

    return $ids;
  }

}
