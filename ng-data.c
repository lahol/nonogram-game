#include "ng-data.h"
#include <stdio.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

struct _NonogramPrivate {
    guint reference_count;
};

void ng_free(Nonogram *ng)
{
    printf("ng_free(%p)\n", ng);
    if (ng == NULL)
        return;
    g_free(ng->row_hints);
    g_free(ng->col_hints);
    g_free(ng->row_offsets);
    g_free(ng->col_offsets);
    g_free(ng->field);
    g_free(ng->priv);
    g_free(ng);
}

void ng_data_ref(Nonogram *ng)
{
    g_return_if_fail(ng != NULL);
    if (ng->priv == NULL)
        ng->priv = g_malloc0(sizeof(NonogramPrivate));
    ++ng->priv->reference_count;
}

void ng_data_unref(Nonogram *ng)
{
    g_return_if_fail(ng != NULL);
    
    if (ng->priv == NULL || ng->priv->reference_count <= 1)
        ng_free(ng);
    else
        --ng->priv->reference_count;
}

gboolean ng_read_data_general(Nonogram *ng, JsonNode *node)
{
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

    tmp = elements;
    for (i = 0; i < count; ++i) {
        if (tmp && JSON_NODE_HOLDS_ARRAY((JsonNode *)tmp->data)) {
            JsonArray *element_array = json_node_get_array((JsonNode *)tmp->data);
            offsets[i + 1] = offsets[i] + json_array_get_length(element_array);
            json_array_foreach_element(element_array, ng_hint_array_read_number_to_array, (gpointer)&(hints[offsets[i]]));
            tmp = g_list_next(tmp);
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

    /* TODO: hintmarks */
    JsonArray *array;
    guint i;
    guint max;
    GList *elements, *tmp;
    if (json_object_has_member(obj, "picture") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(obj, "picture"))) {

        array = json_node_get_array(json_object_get_member(obj, "picture"));

        elements = json_array_get_elements(array);
        for (i = 0, tmp = elements; i < ng->width * ng->height && tmp; ++i, tmp = g_list_next(tmp)) {
            if (!JSON_NODE_HOLDS_VALUE(tmp->data))
                continue;
            ng->field[i] = (guchar)json_node_get_int((JsonNode *)tmp->data);
        }
        g_list_free(elements);
    }

    if (json_object_has_member(obj, "rowmarks") &&
            JSON_NODE_HOLDS_ARRAY(json_object_get_member(obj, "rowmarks"))) {
        array = json_node_get_array(json_object_get_member(obj, "rowmarks"));

        elements = json_array_get_elements(array);
        max = ng->row_offsets ? ng->row_offsets[ng->height] : 0;

        for (i = 0, tmp = elements; i < max && tmp; ++i, tmp = g_list_next(tmp)) {
            if (!JSON_NODE_HOLDS_VALUE(tmp->data))
                continue;
            NG_HINT_SET_STATE(ng->row_hints[i], (guint16)json_node_get_int((JsonNode *)tmp->data));
        }
        g_list_free(elements);
    }

    if (json_object_has_member(obj, "columnmarks") &&
            JSON_NODE_HOLDS_ARRAY(json_object_get_member(obj, "columnmarks"))) {
        array = json_node_get_array(json_object_get_member(obj, "columnmarks"));

        elements = json_array_get_elements(array);
        max = ng->col_offsets ? ng->col_offsets[ng->width] : 0;

        for (i = 0, tmp = elements; i < max && tmp; ++i, tmp = g_list_next(tmp)) {
            if (!JSON_NODE_HOLDS_VALUE(tmp->data))
                continue;
            NG_HINT_SET_STATE(ng->col_hints[i], (guint16)json_node_get_int((JsonNode *)tmp->data));
        }
    }

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

    ng->priv = g_malloc0(sizeof(NonogramPrivate));
    ng->priv->reference_count = 1;

    guint i;
    printf(" row hints (offset) [%d]:", ng->height);
    for (i = 0; i <= ng->height; ++i)
        printf(" %d", ng->row_offsets[i]);
    printf("\n col hints (offset) [%d]:", ng->width);
    for (i = 0; i <= ng->width; ++i)
        printf(" %d", ng->col_offsets[i]);
    printf("\n");

out:
    if (parser)
        g_object_unref(parser);
    return ng;
}

void ng_write_data_general(Nonogram *ng, JsonBuilder *builder)
{
    if (ng == NULL || builder == NULL)
        return;
    json_builder_set_member_name(builder, "general");
    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "width");
    json_builder_add_int_value(builder, ng->width);
    
    json_builder_set_member_name(builder, "height");
    json_builder_add_int_value(builder, ng->height);

    json_builder_set_member_name(builder, "version");
    json_builder_add_string_value(builder, "1.0");

    json_builder_end_object(builder);
}

void ng_write_data_hints(Nonogram *ng, JsonBuilder *builder)
{
    if (ng == NULL || builder == NULL)
        return;

    guint i, j;

    json_builder_set_member_name(builder, "hints");
    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "rows");
    json_builder_begin_array(builder);

    for (i = 0; i < ng->height; ++i) {
        json_builder_begin_array(builder);
        for (j = ng->row_offsets[i]; j < ng->row_offsets[i+1]; ++j) {
            json_builder_add_int_value(builder, NG_HINT_NUM(ng->row_hints[j]));
        }
        json_builder_end_array(builder);
    }

    json_builder_end_array(builder);

    json_builder_set_member_name(builder, "columns");
    json_builder_begin_array(builder);

    for (i = 0; i < ng->width; ++i) {
        json_builder_begin_array(builder);
        for (j = ng->col_offsets[i]; j < ng->col_offsets[i+1]; ++j) {
            json_builder_add_int_value(builder, NG_HINT_NUM(ng->col_hints[j]));
        }
        json_builder_end_array(builder);
    }

    json_builder_end_array(builder);

    json_builder_end_object(builder);
}

void ng_write_data_state(Nonogram *ng, JsonBuilder *builder)
{
    if (ng == NULL || builder == NULL)
        return;
    json_builder_set_member_name(builder, "state");
    json_builder_begin_object(builder);

    guint i, j;
    json_builder_set_member_name(builder, "rowmarks");
    json_builder_begin_array(builder);
    /* TODO: really implement hintmarks */
    for (i = 0; i < ng->height; ++i) {
        for (j = ng->row_offsets[i]; j < ng->row_offsets[i+1]; ++j) {
            json_builder_add_int_value(builder, NG_HINT_STATE(ng->row_hints[j]));
        }
    }
    json_builder_end_array(builder);

    json_builder_set_member_name(builder, "columnmarks");
    json_builder_begin_array(builder);
    for (i = 0; i < ng->width; ++i) {
        for (j = ng->col_offsets[i]; j < ng->col_offsets[i+1]; ++j) {
            json_builder_add_int_value(builder, NG_HINT_STATE(ng->col_hints[j]));
        }
    }
    json_builder_end_array(builder);

    json_builder_set_member_name(builder, "picture");
    json_builder_begin_array(builder);
    for (i = 0; i < ng->width * ng->height; ++i) {
        json_builder_add_int_value(builder, ng->field[i]);
    }
    json_builder_end_array(builder);

    json_builder_end_object(builder);
}

void ng_write_data_to_file(Nonogram *ng, gchar *filename)
{
    if (ng == NULL || filename == NULL)
        return;

    JsonBuilder *builder = json_builder_new();
    JsonNode *root;

    json_builder_begin_object(builder);
    ng_write_data_general(ng, builder);
    ng_write_data_hints(ng, builder);
    ng_write_data_state(ng, builder);
    json_builder_end_object(builder);

    root = json_builder_get_root(builder);
    g_object_unref(builder);

    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);

    json_generator_to_file(gen, filename, NULL);

    json_node_free(root);
    g_object_unref(gen);
}

/* fills an area starting from point (x,y) cx units wide, cy units high
 * with value (for monochrom: 1: fill, 0: clear, 0xff: “lock” later more colors) */
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

void ng_toggle_hint(Nonogram *ng, NgHintType type, guint16 offset)
{
    if (ng == NULL)
        return;
    if (type == NG_HINT_ROW) {
        if (ng->row_offsets == NULL)
            return;
        if (ng->row_offsets[ng->height] <= offset)
            return;
        NG_HINT_SET_STATE(ng->row_hints[offset], 1 - NG_HINT_STATE(ng->row_hints[offset]));
    }
    else if (type == NG_HINT_COLUMN) {
        if (ng->col_offsets == NULL)
            return;
        if (ng->col_offsets[ng->width] <= offset)
            return;
        NG_HINT_SET_STATE(ng->col_hints[offset], 1 - NG_HINT_STATE(ng->col_hints[offset]));
    }
}
