<?php

/**
 * @file
 * Contains SearchApiEtRemoveLanguageFromItemId.
 */

/**
 * Processor for removing language prefix from indexed multilingual item ids.
 */
class SearchApiEtRemoveLanguageFromItemId extends SearchApiAbstractProcessor {

  /**
   * {@inheritdoc}
   */
  public function supportsIndex(SearchApiIndex $index) {
    // This processor supports only multilingual indexes.
    $controller = search_api_get_datasource_controller($index->item_type);
    if ($controller instanceof SearchApiEtDatasourceController) {
      return TRUE;
    }
  }

  /**
   * {@inheritdoc}
   */
  public function configurationForm() {
    // No configuration at all for this processor.
    return;
  }

  /**
   * {@inheritdoc}
   */
  public function postprocessSearchResults(array &$response, SearchApiQuery $query) {
    if (!empty($response['results']) && is_array($response['results'])) {
      foreach ($response['results'] as $key => $result) {
        if (strpos($result['id'], '_') !== FALSE) {
          list($item_language, $item_id) = explode('_', $result['id']);
          $response['results'][$item_id]['id'] = $item_id;
          unset($response['results'][$key]);
        }
      }
    }
  }

}
