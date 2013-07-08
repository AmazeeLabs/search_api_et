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

    // If some items couldn't be loaded, remove them from tracking.
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
          // @todo Remove data of other languages from fields?
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
    // already in place (and only one value is allowed), we have to employ this
    // trick to make it work.
    $property_info_alter = isset($info['property info alter']) ? $info['property info alter'] : NULL;
    $callback = new SearchApiEtPropertyInfoAlter($property_info_alter);
    $info['property info alter'] = array($callback, 'propertyInfoAlter');
    return entity_metadata_wrapper($this->entityType, $item, $info);
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
    if (!$this->table) {
      return;
    }
    // Get all existing IDs, in case we are just

    $all_ids = $this->getAllItemIds();
    $this->trackItemInsert($new_ids, $indexes);
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
      // @todo
    }

    return $ids;
  }

}
