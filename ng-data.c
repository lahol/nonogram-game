#include "ng-data.h"
#include <stdio.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

void ng_free(Nonogram *ng)
{
    if (ng == NULL)
        return;
    g_free(ng->row_hints);
    g_free(ng->col_hints);
    g_free(ng->row_offsets);
    g_free(ng->col_offsets);
    g_free(ng->field);
    g_free(ng);
}

gboolean ng_read_data_general(Nonogram *ng, JsonNode *node)
{
    printf("read data general\n");
    if (!JSON_NODE_HOLDS_OBJECT(node) || ng == NULL)
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    if (!json_object_has_member(obj, "width") ||
            !json_object_has_member(obj, "height"))
        return FALSE;

    ng->width = (guint16)json_object_get_int_member(obj, "width");
    ng->height = (guint16)json_object_get_int_member(obj, "height");

    if (ng->width == 0 || ng->height == 0)
        return FALSE;

    ng->field = g_malloc0(sizeof(guchar) * ng->height * ng->width);

    return TRUE;
}

void ng_hint_array_advance_total_count(JsonArray *array, guint index, JsonNode *element_node, gpointer userdata)
{
    if (!JSON_NODE_HOLDS_ARRAY(element_node))
        return;
    if (userdata)
        *(guint *)userdata += json_array_get_length(json_node_get_array(element_node));
}

void ng_hint_array_read_number_to_array(JsonArray *array, guint index, JsonNode *element_node, gpointer userdata)
{
    if (!JSON_NODE_HOLDS_VALUE(element_node))
        return;
    if (userdata)
        ((guint32 *)userdata)[index] = (guint32)json_node_get_int(element_node);
}

void ng_hint_array_read_data(JsonArray *array, guint32 *hints, guint16 *offsets, guint32 count)
{
    GList *elements = json_array_get_elements(array);
    GList *tmp;
    guint32 i;

    for (tmp = elements, i = 0; tmp != NULL && i < count; tmp = g_list_next(tmp), ++i) {
        if (JSON_NODE_HOLDS_ARRAY((JsonNode *)tmp->data)) {
            JsonArray *element_array = json_node_get_array((JsonNode *)tmp->data);
            offsets[i + 1] = offsets[i] + json_array_get_length(element_array);
            json_array_foreach_element(element_array, ng_hint_array_read_number_to_array, (gpointer)&(hints[offsets[i]]));
        }
        else {
            offsets[i + 1] = offsets[i];
        }
    }

    g_list_free(elements);
}

gboolean ng_read_data_hints(Nonogram *ng, JsonNode *node)
{
    printf("read data hints\n");
    if (!JSON_NODE_HOLDS_OBJECT(node) || ng == NULL)
        return FALSE;
    JsonObject *obj = json_node_get_object(node);

    if (!json_object_has_member(obj, "rows") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(obj, "rows")) ||
            !json_object_has_member(obj, "columns") || !JSON_NODE_HOLDS_ARRAY(json_object_get_member(obj, "columns")))
        return FALSE;
    ng->row_offsets = g_malloc0(sizeof(guint16) * (ng->height + 1));
    ng->col_offsets = g_malloc0(sizeof(guint16) * (ng->width + 1));
    
    JsonArray *array;
    guint num_hints = 0;
    array = json_node_get_array(json_object_get_member(obj, "rows"));
    if (json_array_get_length(array) > ng->height)
        return FALSE;
    json_array_foreach_element(array, ng_hint_array_advance_total_count, (gpointer)&num_hints);
    if (num_hints > 0) {
        ng->row_hints = g_malloc0(sizeof(guint32) * num_hints);
        ng_hint_array_read_data(array, ng->row_hints, ng->row_offsets, ng->height);
    }

    num_hints = 0;
    array = json_node_get_array(json_object_get_member(obj, "columns"));
    if (json_array_get_length(array) > ng->width)
        return FALSE;
    json_array_foreach_element(array, ng_hint_array_advance_total_count, (gpointer)&num_hints);
    if (num_hints > 0) {
        ng->col_hints = g_malloc0(sizeof(guint32) * num_hints);
        ng_hint_array_read_data(array, ng->col_hints, ng->col_offsets, ng->width);
    }

    return TRUE;
}

gboolean ng_read_data_state(Nonogram *ng, JsonNode *node)
{
    printf("read data state\n");
    if (!JSON_NODE_HOLDS_OBJECT(node) || ng == NULL)
        return FALSE;
    JsonObject *obj = json_node_get_object(node);
    return TRUE;
}

Nonogram *ng_read_data_from_file(gchar *filename)
{
    printf("read data from file\n");
    JsonParser *parser = NULL;
    JsonNode *root = NULL;
    JsonObject *obj;
    Nonogram *ng = NULL;
    GError *err = NULL;

    parser = json_parser_new();
    if (!json_parser_load_from_file(parser, filename, &err)) {
        if (err) {
            printf("error: %s\n", err->message);
            g_error_free(err);
        }
        goto out;
    }

    root = json_parser_get_root(parser);
    obj = json_node_get_object(root);

    ng = g_malloc0(sizeof(Nonogram));

    if (!ng_read_data_general(ng, json_object_get_member(obj, "general"))) {
        ng_free(ng);
        ng = NULL;
        goto out;
    }

    if (!ng_read_data_hints(ng, json_object_get_member(obj, "hints"))) {
        ng_free(ng);
        ng = NULL;
        goto out;
    }

    if (json_object_has_member(obj, "state") &&
            !ng_read_data_state(ng, json_object_get_member(obj, "state"))) {
        ng_free(ng);
        ng = NULL;
        goto out;
    }

out:
    if (parser)
        g_object_unref(parser);
    return ng;
}

void ng_write_data_to_file(Nonogram *ng, gchar *filename)
{
}

/* fills an area starting from point (x,y) cx units wide, cy units high
 * with value (for monochrom: 1: fill, 0: clear, 2: “lock” later more colors) */
void ng_fill_area(Nonogram *ng, guint16 x, guint16 y, guint16 cx, guint16 cy, guchar value)
{
    if (ng == NULL)
        return;
    if (x >= ng->width || y >= ng->height || cx == 0 || cy == 0)
        return;
    gint i, j;
    for (j = y; j < y + cy && j < ng->height; ++j) {
        for (i = x; i < x + cx && i < ng->width; ++i) {
            ng->field[j * ng->width + i] = value;
        }
    }
}
