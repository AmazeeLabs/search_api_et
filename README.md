This is a modification of the [Search API Entity Translation](https://www.drupal.org/project/search_api_et) module that only supports "Languages to be included in the index" => "all site languages" mode.

## What was changed

Originally, the module watches entity translations in hook_entity_translation_{insert|update|delete}, accordingly inserting/updating/deleting items in the search index.

The was one implication when the "Languages to be included in the index" => "all site languages" mode is used. If you just created an entity, and translated it to some languages, only existing translations will go to the search index. But if then you re-queue search items (you can enable/disable the readonly mode to do this), items for all site languages appear in the search index.

The code was changed to watch hook_entity_{insert|update|delete} and always process all site languages for an entity. It was not tested how the module would behave in modes other than "all site languages".

## Why it was not fixed in general way

With the current design (it includes both original and patched module versions), it's not really possible to correctly track items. For example, we have two multilingual node search indexes. One has the "all site languages" mode, and the other one has the "all entity languages" mode. When we track item with the search_api_track_item_{delete|insert} function, we pass the item IDs, that are already language prefixed, but we can't pass the target index in there. So, the search_api will decide itself to which indexes the item should be passed. An the question which is not answered here: which item IDs should we pass, "all site languages" or "all entity languages"?

To fix this, it can be possible to not use search_api_track_item_{delete|insert} methods, but this would mean that the module will do the search_api job.
