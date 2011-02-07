#include "mesh.h"
// Revision: 7.2.2011

// Read groupGroup (both the un/structured)
void read_groupgroup (hid_t file_id, const char* path, groupgroup_t *groupgroup)
{
    int nb_dims;
    H5T_class_t type_class;
    size_t length;
    char success = FALSE;

    groupgroup->nb_groupgroupnames = 1;  // in case of single value
    groupgroup->name = get_name_from_path(path);
    if (H5Lexists(file_id, path, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path, &nb_dims) >= 0)
            if (nb_dims <= 1)
                if (H5LTget_dataset_info(file_id, path, &(groupgroup->nb_groupgroupnames), &type_class, &length) >= 0)
                    if (type_class == H5T_STRING)
                        if(read_string_dataset(file_id, path, groupgroup->nb_groupgroupnames, length, &(groupgroup->groupgroupnames)))
                            success = TRUE;
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path);
        groupgroup->nb_groupgroupnames = 0;
        groupgroup->groupgroupnames = NULL;
    }
}


// Read m x n dataset "/x, /y or /z" (32-bit signed float)
void read_smesh_axis (hid_t file_id, const char* path, axis_t *axis)
{
    H5T_class_t type_class;
    int nb_dims;
    size_t length;
    char success = FALSE;
    hid_t dset_id;

    axis->nb_nodes = 1;
    if (H5Lexists(file_id, path, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path, &nb_dims) >= 0)
            if (nb_dims <= 1)
                if (H5LTget_dataset_info(file_id, path, &(axis->nb_nodes), &type_class, &length) >= 0)
                    if (type_class == H5T_FLOAT && length == 4)
                    {
                        axis->nodes = (float *) malloc((size_t) axis->nb_nodes * sizeof(float));
                        dset_id = H5Dopen(file_id, path, H5P_DEFAULT);
                        if (H5Dread(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, axis->nodes) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path);
                        H5Dclose(dset_id);
                        success = TRUE;
                    }
    if (!success)
    {
        printf("***** WARNING(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path);
        axis->nb_nodes = 0;
        axis->nodes = NULL;
    }

}


// Read group in structured mesh (+normals)
void read_smesh_group (hid_t file_id, const char *path, sgroup_t *sgroup)
{
    hid_t sgroup_id;
    hsize_t dims[2] = {1, 1}, i;
    int nb_dims;
    H5T_class_t type_class;
    size_t length;
    char success = FALSE;
    char success2 = FALSE;
    char *temp;
    char *normalpath;

    normalpath = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));

    sgroup->name = get_name_from_path(path);
    if (!read_string_attribute(file_id, path, A_TYPE, &(sgroup->type)))
        printf("***** ERROR(%s): Cannot find mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_TYPE);
    if (!read_string_attribute(file_id, path, A_ENTITY_TYPE, &(sgroup->entitytype)))
        printf("***** ERROR(%s): Cannot find mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_ENTITY_TYPE);
    if (H5Lexists(file_id, path, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path, &nb_dims) >= 0)
            if (nb_dims == 2)
                if (H5LTget_dataset_info(file_id, path, dims, &type_class, &length) >= 0)
                    if ((dims[1] == 2 || dims[1] == 4 || dims[1] == 6) && type_class == H5T_INTEGER && length == 4)
                    {
                        sgroup->elements = (unsigned int **) malloc((size_t) (dims[0] * sizeof(unsigned int*)));
                        sgroup->elements[0] = (unsigned int *) malloc((size_t) (dims[0] * dims[1] * sizeof(unsigned int)));
                        for (i = 1; i < dims[0]; i++)
                            sgroup->elements[i] = sgroup->elements[0] + i * dims[1];
                        sgroup_id = H5Dopen(file_id, path, H5P_DEFAULT);
                        if (H5Dread(sgroup_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, sgroup->elements[0]) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path);
                        H5Dclose(sgroup_id);
                        sgroup->nb_elements = dims[0];
                        sgroup->nb_dims = dims[1] / 2;  // nb_dims = number of real dimensions, dims[1] = number of columns
                        success = TRUE;
                    }
    if (success)
    {
        strcpy(normalpath, path);
        temp = strstr(path, "/group/");
        normalpath[temp-path] = '\0';
        temp = get_name_from_path(path);
        strcat(normalpath, G_NORMAL);
        strcat(normalpath, "/");
        strcat(normalpath, temp);
        free(temp);

        dims[0] = 1;
        dims[1] = 1;
        if (H5Lexists(file_id, normalpath, H5P_DEFAULT) > 0)
            if (H5LTget_dataset_ndims(file_id, normalpath, &nb_dims) >= 0)
                if (nb_dims <= 1)
                    if (H5LTget_dataset_info(file_id, normalpath, dims, &type_class, &length) >= 0)
                        if (dims[0] == sgroup->nb_elements && type_class == H5T_STRING && length == 2)
                            if(read_string_dataset(file_id, normalpath, dims[0], length, &(sgroup->normals)))
                                success2 = TRUE;
        if (!success2)
            sgroup->normals = NULL;
    }
    else
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path);
        sgroup->nb_dims = 0;
        sgroup->nb_elements = 0;
        sgroup->elements = NULL;
        sgroup->normals = NULL;
    }
    free(normalpath);
}

// Read table of type "pointInElement" from /selectorOnMesh (structured) (element: 32-bit unsigned int, vector: 32-bit signed float)
void read_ssom_pie_table (hid_t file_id, const char *path, ssom_pie_table_t *ssom_pie_table)
{
    hsize_t nfields, nrecords, i;
    char success = FALSE;
    char **field_names;
    size_t *field_sizes;
    size_t *field_offsets;
    size_t type_size;
    int field_index1[] = {0, 1, 2, 3, 4, 5};
    int field_index2[3];

    ssom_pie_table->name = get_name_from_path(path);
    if (H5Lexists(file_id, path, H5P_DEFAULT) != FALSE)
        if (H5TBget_table_info(file_id, path, &nfields, &nrecords) >= 0)
            if ((nfields == 3 || nfields == 6 || nfields == 9) && nrecords > 0)
            {
                field_names = (char **) malloc(nfields * sizeof(char *));
                field_names[0] = (char *) malloc(TABLE_FIELD_NAME_LENGTH * nfields * sizeof(char));
                for (i = 0; i < nfields; i++)
                    field_names[i] = field_names[0] + i * TABLE_FIELD_NAME_LENGTH;
                field_sizes = (size_t *) malloc((size_t) sizeof(size_t *) * nfields);
                field_offsets = (size_t *) malloc((size_t) sizeof(size_t *) * nfields);

                if (H5TBget_field_info(file_id, path, field_names, field_sizes, field_offsets, &type_size) >= 0)
                {
                    if (nfields == 3)
                    {
                        if (strcmp(field_names[0], "imin") == 0 && strcmp(field_names[1], "imax") == 0 && strcmp(field_names[2], "v1") == 0)
                        {
                            success = TRUE;
                            field_index2[0] = 2;
                        }
                    }
                    else if (nfields == 6)
                    {
                        if (strcmp(field_names[0], "imin") == 0 && strcmp(field_names[1], "jmin") == 0 && strcmp(field_names[2], "imax") == 0
                                && strcmp(field_names[3], "jmax") == 0 && strcmp(field_names[4], "v1") == 0 && strcmp(field_names[5], "v2") == 0)
                        {
                            success = TRUE;
                            field_index2[0] = 4;
                            field_index2[1] = 5;
                        }
                    }
                    else if (nfields == 9)
                    {
                        if (strcmp(field_names[0], "imin") == 0 && strcmp(field_names[1], "jmin") == 0 && strcmp(field_names[2], "kmin") == 0
                                && strcmp(field_names[3], "imax") == 0 && strcmp(field_names[4], "jmax") == 0 && strcmp(field_names[5], "kmax") == 0
                                && strcmp(field_names[6], "v1") == 0 && strcmp(field_names[7], "v2") == 0 && strcmp(field_names[8], "v3") == 0)
                        {
                            success = TRUE;
                            field_index2[0] = 6;
                            field_index2[1] = 7;
                            field_index2[2] = 8;
                        }
                    }
                }
                if (success == TRUE)
                {
                    ssom_pie_table->nb_dims = nfields / 3;
                    ssom_pie_table->elements = (unsigned int **) malloc((size_t) nrecords * sizeof(unsigned int *));
                    ssom_pie_table->elements[0] = (unsigned int *) malloc((size_t) nrecords * 2 * (size_t) ssom_pie_table->nb_dims * sizeof(unsigned int));
                    ssom_pie_table->vectors = (float **) malloc((size_t) nrecords * sizeof(float *));
                    ssom_pie_table->vectors[0] = (float *) malloc((size_t) nrecords * ssom_pie_table->nb_dims * sizeof(float));
                    for (i = 1; i < nrecords; i++)
                    {
                        ssom_pie_table->elements[i] = ssom_pie_table->elements[0] + i * 2 * ssom_pie_table->nb_dims;
                        ssom_pie_table->vectors[i] = ssom_pie_table->vectors[0] + i * ssom_pie_table->nb_dims;
                    }

                    if (H5TBread_fields_index(file_id, path, (int) ssom_pie_table->nb_dims * 2, field_index1, 0, nrecords, (hsize_t) ssom_pie_table->nb_dims * 2 * sizeof(int), field_offsets, field_sizes, ssom_pie_table->elements[0])
                            < 0
                            ||
                            H5TBread_fields_index(file_id, path, (int) ssom_pie_table->nb_dims, field_index2, 0, nrecords, (hsize_t) ssom_pie_table->nb_dims * sizeof(float), field_offsets, field_sizes, ssom_pie_table->vectors[0])
                            < 0)
                        printf("***** WARNING(%s): Data from table \"%s\" may be corrupted. *****\n\n", C_MESH, path);
                    ssom_pie_table->nb_points = nrecords;
                }
                free(field_names[0]);
                free(field_names);
                free(field_sizes);
                free(field_offsets);
            }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read table \"%s\". *****\n\n", C_MESH, path);
        ssom_pie_table->nb_dims = 0;
        ssom_pie_table->nb_points = 0;
        ssom_pie_table->elements = NULL;
        ssom_pie_table->vectors = NULL;
    }
}


// Read structured mesh
void read_smesh(hid_t file_id, const char* path, smesh_t *smesh)
{
    hsize_t i;
    children_t children;
    char *path2, *path3, *type;
    char success;

    path2 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));
    path3 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));

    // X Axis
    strcpy(path2, path);
    strcat(path2, G_CARTESIAN_GRID);
    strcat(path2, G_X);
    read_smesh_axis(file_id, path2, &(smesh->x));

    // Y Axis
    strcpy(path2, path);
    strcat(path2, G_CARTESIAN_GRID);
    strcat(path2, G_Y);
    read_smesh_axis(file_id, path2, &(smesh->y));

    // Z Axis
    strcpy(path2, path);
    strcat(path2, G_CARTESIAN_GRID);
    strcat(path2, G_Z);
    read_smesh_axis(file_id, path2, &(smesh->z));

    // groups
    strcpy(path2, path);
    strcat(path2, G_GROUP);
    children = read_children_name(file_id, path2);
    smesh->nb_groups = children.nb_children;
    smesh->groups = NULL;
    if (children.nb_children > 0)
    {
        smesh->groups = (sgroup_t *) malloc((size_t) children.nb_children * sizeof(sgroup_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            read_smesh_group(file_id, path3, smesh->groups + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
    }

    // read groupGroup if exists
    strcpy(path2, path);
    strcat(path2, G_GROUPGROUP);
    children = read_children_name(file_id, path2);
    smesh->nb_groupgroups = children.nb_children;
    smesh->groupgroups = NULL;
    if (children.nb_children > 0)
    {
        smesh->groupgroups = (groupgroup_t *) malloc((size_t) children.nb_children * sizeof(groupgroup_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            read_groupgroup(file_id, path3, smesh->groupgroups + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
    }

    // read selectorOnMesh
    strcpy(path2, path);
    strcat(path2, G_SELECTOR_ON_MESH);
    children = read_children_name(file_id, path2);
    smesh->nb_som_tables = children.nb_children;
    smesh->som_tables = NULL;
    if (children.nb_children > 0)
    {
        smesh->som_tables = (ssom_pie_table_t *) malloc((size_t) children.nb_children * sizeof(ssom_pie_table_t));
        for (i = 0; i < children.nb_children; i++)
        {
            success = FALSE;
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            if (read_string_attribute(file_id, path3, A_TYPE, &type))
            {
                if (strcmp(type,V_POINT_IN_ELEMENT) == 0)
                {
                    read_ssom_pie_table(file_id, path3, smesh->som_tables + i);
                    success = TRUE;
                }
                free(type);
            }
            if (!success)
            {
                printf("***** ERROR(%s): Cannot find mandatory attribute \"%s\" in \"%s\". *****\n\n", C_MESH, A_TYPE, path3);
                smesh->som_tables[i].name = NULL;
                smesh->som_tables[i].nb_dims = 0;
                smesh->som_tables[i].nb_points = 0;
                smesh->som_tables[i].elements = NULL;
                smesh->som_tables[i].vectors = NULL;
            }
            free(children.childnames[i]);
        }
        free(children.childnames);
    }

    free(path3);
    free(path2);
}


// Read group in unstructured mesh
void read_umesh_group (hid_t file_id, const char* path, ugroup_t *ugroup)
{
    hid_t dset_id;
    int nb_dims;
    H5T_class_t type_class;
    size_t length;
    char success = FALSE;

    ugroup->nb_groupelts = 1;
    ugroup->name = get_name_from_path(path);
    if (H5Lexists(file_id, path, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path, &nb_dims) >= 0)
            if (nb_dims <= 1)
                if (H5LTget_dataset_info(file_id, path, &(ugroup->nb_groupelts), &type_class, &length) >= 0)
                    if (type_class == H5T_INTEGER && length == 4)
                    {
                        ugroup->groupelts = (int *) malloc((size_t) (ugroup->nb_groupelts * sizeof(int)));
                        dset_id = H5Dopen(file_id, path, H5P_DEFAULT);
                        if (H5Dread(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, ugroup->groupelts) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path);
                        H5Dclose(dset_id);
                        success = TRUE;
                    }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path);
        ugroup->nb_groupelts = 0;
        ugroup->groupelts = NULL;
    }
}


// Read table of type "pointInElement" from /selectorOnMesh (unstructured) (index: 32-bit signed int, vector: 32-bit signed float)
void read_usom_pie_table (hid_t file_id, const char *path, usom_pie_table_t *usom_pie_table)
{
    hsize_t nfields, nrecords, i;
    char success = FALSE;
    char **field_names;
    size_t *field_sizes;
    size_t *field_offsets;
    size_t type_size;
    int field_index1[] = {0};
    int field_index2[] = {1, 2, 3};

    if (H5Lexists(file_id, path, H5P_DEFAULT) != FALSE)
        if (H5TBget_table_info(file_id, path, &nfields, &nrecords) >= 0)
            if (nfields >= 2 && nfields <=4 && nrecords > 0)
            {
                field_names = (char **) malloc(nfields * sizeof(char *));
                field_names[0] = (char *) malloc(TABLE_FIELD_NAME_LENGTH * nfields * sizeof(char));
                for (i = 0; i < nfields; i++)
                    field_names[i] = field_names[0] + i * TABLE_FIELD_NAME_LENGTH;
                field_sizes = (size_t *) malloc((size_t) sizeof(size_t *) * nfields);
                field_offsets = (size_t *) malloc((size_t) sizeof(size_t *) * nfields);

                if (H5TBget_field_info(file_id, path, field_names, field_sizes, field_offsets, &type_size) >= 0)
                    if (strcmp(field_names[0], "index") == 0)
                    {
                        usom_pie_table->indices = (int *) malloc((size_t) nrecords * sizeof(int));
                        usom_pie_table->vectors = (float **) malloc((size_t) nrecords * sizeof(float *));
                        usom_pie_table->vectors[0] = (float *) malloc((size_t) nrecords * (nfields - 1) * sizeof(float));
                        for (i = 1; i < nrecords; i++)
                            usom_pie_table->vectors[i] = usom_pie_table->vectors[0] + i * (nfields - 1);
                        if (H5TBread_fields_index(file_id, path, 1, field_index1, 0, nrecords, sizeof(int), field_offsets, field_sizes, usom_pie_table->indices)
                                < 0
                                ||
                                H5TBread_fields_index(file_id, path, (nfields - 1), field_index2, 0, nrecords, (nfields - 1) * sizeof(float), field_offsets, field_sizes, usom_pie_table->vectors[0])
                                < 0)
                            printf("***** WARNING(%s): Data from table \"%s\" may be corrupted. *****\n\n", C_MESH, path);
                        usom_pie_table->nb_points = nrecords;
                        usom_pie_table->nb_dims = (char) nfields - 1;
                        success = TRUE;
                    }
                free(field_names[0]);
                free(field_names);
                free(field_sizes);
                free(field_offsets);
            }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read table \"%s\". *****\n\n", C_MESH, path);
        usom_pie_table->nb_points = 0;
        usom_pie_table->nb_dims = 0;
        usom_pie_table->indices = NULL;
        usom_pie_table->vectors = NULL;
    }
}


// Read dataset of type "edge" or "face" from /selectorOnMesh (32-bit signed int)
void read_usom_ef_table (hid_t file_id, const char *path, usom_ef_table_t *usom_ef_table)
{
    hsize_t dims[2] = {1, 1}, i;
    int nb_dims;
    H5T_class_t type_class;
    size_t length;
    hid_t dset_id;
    char success = FALSE;
    if (H5Lexists(file_id, path, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path, &nb_dims) >= 0)
            if (nb_dims == 2)
                if (H5LTget_dataset_info(file_id, path, dims, &type_class, &length) >= 0)
                    if (type_class == H5T_INTEGER && length == 4)
                    {
                        usom_ef_table->items = (int **) malloc((size_t) dims[0] * sizeof(int *));
                        usom_ef_table->items[0] = (int *) malloc((size_t) dims[0] * dims[1] * sizeof(int));
                        for (i = 1; i < dims[0]; i++)
                            usom_ef_table->items[i] = usom_ef_table->items[0] + i * dims[1];
                        dset_id = H5Dopen(file_id, path, H5P_DEFAULT);
                        if (H5Dread(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, usom_ef_table->items[0]) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path);
                        H5Dclose(dset_id);
                        usom_ef_table->nb_items = dims[0];
                        success = TRUE;
                    }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path);
        usom_ef_table->nb_items = 0;
        usom_ef_table->items = NULL;
    }
}


// Read selector on mesh (unstructured mesh)
void read_umesh_som_table (hid_t file_id, const char *path, usom_table_t *usom_table)
{
    char *type;

    usom_table->name = get_name_from_path(path);
    if (read_string_attribute(file_id, path, A_TYPE, &type))
    {
        if (strcmp(type, V_POINT_IN_ELEMENT) == 0)
        {
            usom_table->type = SOM_POINT_IN_ELEMENT;
            read_usom_pie_table(file_id, path, &(usom_table->data.pie));
        }

        else if (strcmp(type, V_EDGE) == 0)
        {
            usom_table->type = SOM_EDGE;
            read_usom_ef_table(file_id, path, &(usom_table->data.ef));
        }
        else if (strcmp(type, V_FACE) == 0)
        {
            usom_table->type = SOM_FACE;
            read_usom_ef_table(file_id, path, &(usom_table->data.ef));
        }
        free(type);
    }
    else
        printf("***** ERROR(%s): Cannot find mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_TYPE);
}


// Read unstructured mesh
char read_umesh (hid_t file_id, const char* path, umesh_t *umesh)
{
    char *path2, *path3, rdata = TRUE;
    hsize_t dims[2] = {1, 1}, i;
    int nb_dims;
    H5T_class_t type_class;
    size_t length;
    char success = FALSE;
    hid_t dset_id;
    children_t children;

    path2 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));
    path3 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));

    // Read m x 1 dataset "elementNodes" (32-bit signed integer)
    umesh->nb_elementnodes = 1;
    strcpy(path2, path);
    strcat(path2, G_ELEMENT_NODES);
    if (H5Lexists(file_id, path2, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path2, &nb_dims) >= 0)
            if (nb_dims <= 1)
                if (H5LTget_dataset_info(file_id, path2, &(umesh->nb_elementnodes), &type_class, &length) >= 0)
                    if (type_class == H5T_INTEGER && length == 4)
                    {
                        umesh->elementnodes = (int *) malloc((size_t) (umesh->nb_elementnodes * sizeof(int)));
                        dset_id = H5Dopen(file_id, path2, H5P_DEFAULT);
                        if (H5Dread(dset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, umesh->elementnodes) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path2);
                        H5Dclose(dset_id);
                        success = TRUE;
                    }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path2);
        umesh->nb_elementnodes = 0;
        umesh->elementnodes = NULL;
        rdata = FALSE;
    }

    success = FALSE;

    // Read m x 1 dataset "elementTypes" (8-bit signed char)
    umesh->nb_elementtypes = 1;
    strcpy(path2, path);
    strcat(path2, G_ELEMENT_TYPES);
    if (H5Lexists(file_id, path2, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path2, &nb_dims) >= 0)
            if (nb_dims <= 1)
                if (H5LTget_dataset_info(file_id, path2, &(umesh->nb_elementtypes), &type_class, &length) >= 0)
                    if (type_class == H5T_INTEGER && length == 1)
                    {
                        umesh->elementtypes = (char *) malloc((size_t) (umesh->nb_elementtypes * sizeof(char)));
                        dset_id = H5Dopen(file_id, path2, H5P_DEFAULT);
                        if (H5Dread(dset_id, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, umesh->elementtypes) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path2);
                        H5Dclose(dset_id);
                        success = TRUE;
                    }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path2);
        umesh->nb_elementtypes = 0;
        umesh->elementtypes = NULL;
        rdata = FALSE;
    }

    success = FALSE;

    // Read m x n dataset "nodes" (32-bit signed float)
    strcpy(path2, path);
    strcat(path2, G_NODES);
    if (H5Lexists(file_id, path2, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path2, &nb_dims) >= 0)
            if (nb_dims == 2)
                if (H5LTget_dataset_info(file_id, path2, dims, &type_class, &length) >= 0)
                    if (dims[1] <= 3 && type_class == H5T_FLOAT && length == 4)
                    {
                        umesh->nodes = (float **) malloc((size_t) (dims[0] * sizeof(float *)));
                        umesh->nodes[0] = (float *) malloc(dims[0] * dims[1] * sizeof(float));
                        for (i = 1; i < dims[0]; i++)
                            umesh->nodes[i] = umesh->nodes[0] + i * dims[1];
                        dset_id = H5Dopen(file_id, path2, H5P_DEFAULT);
                        if (H5Dread(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, umesh->nodes[0]) < 0)
                            printf("***** WARNING(%s): Data from dataset \"%s\" may be corrupted. *****\n\n", C_MESH, path2);
                        H5Dclose(dset_id);
                        umesh->nb_nodes[0] = dims[0];
                        umesh->nb_nodes[1] = dims[1];
                        success = TRUE;
                    }
    if (!success)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path2);
        umesh->nb_nodes[0] = 0;
        umesh->nb_nodes[1] = 0;
        umesh->nodes = NULL;
        rdata = FALSE;
    }

    // read groupGroup if exists
    strcpy(path2, path);
    strcat(path2, G_GROUPGROUP);
    children = read_children_name(file_id, path2);
    umesh->nb_groupgroups = children.nb_children;
    umesh->groupgroups = NULL;
    if (children.nb_children > 0)
    {
        umesh->groupgroups = (groupgroup_t *) malloc((size_t) children.nb_children * sizeof(groupgroup_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            read_groupgroup(file_id, path3, umesh->groupgroups + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
    }

    // read group
    strcpy(path2, path);
    strcat(path2, G_GROUP);
    children = read_children_name(file_id, path2);
    umesh->nb_groups = children.nb_children;
    umesh->groups = NULL;
    if (children.nb_children > 0)
    {
        umesh->groups = (ugroup_t *) malloc((size_t) children.nb_children * sizeof(ugroup_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            read_umesh_group(file_id, path3, umesh->groups + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
    }

    // read selectorOnMesh
    strcpy(path2, path);
    strcat(path2, G_SELECTOR_ON_MESH);
    children = read_children_name(file_id, path2);
    umesh->nb_som_tables = children.nb_children;
    umesh->som_tables = NULL;
    if (children.nb_children > 0)
    {
        umesh->som_tables = (usom_table_t *) malloc((size_t) children.nb_children * sizeof(usom_table_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            read_umesh_som_table(file_id, path3, umesh->som_tables + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
    }
    free(path3);
    free(path2);
    return rdata;
}


// Read mesh instance
void read_mesh_instance (hid_t file_id, const char *path, mesh_instance_t *mesh_instance)
{
    char *type;

    mesh_instance->name = get_name_from_path(path);
    if (read_string_attribute(file_id, path, A_TYPE, &type))
    {
        if (strcmp(type, V_STRUCTURED) == 0)
        {
            mesh_instance->type = MSH_STRUCTURED;
            read_smesh(file_id, path, &(mesh_instance->data.structured));
        }
        else if (strcmp(type, V_UNSTRUCTURED) == 0)
        {
            mesh_instance->type = MSH_UNSTRUCTURED;
            read_umesh(file_id, path, &(mesh_instance->data.unstructured));
        }
        else
        {
            mesh_instance->type = MSH_INVALID;
            printf("***** ERROR(%s): Unexpected attribute value of \"%s@%s\". *****\n\n", C_MESH, path, A_TYPE);
        }
        free(type);
    }
    else
    {
        mesh_instance->type = MSH_INVALID;
        printf("***** ERROR(%s): Cannot find mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_TYPE);
    }
}


// Read meshLink instance
void read_meshlink_instance (hid_t file_id, const char *path, meshlink_instance_t *meshlink_instance)
{
    char *path2, *type, success = TRUE, dataset_read = FALSE;
    H5T_class_t type_class;
    size_t length;
    int nb_dims;

    path2 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));
    meshlink_instance->name = get_name_from_path(path);
    if (!read_string_attribute(file_id, path, A_MESH1, &(meshlink_instance->mesh1)))
    {
        printf("***** ERROR(%s): Cannot read mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_MESH1);
        success = FALSE;
    }
    if (!read_string_attribute(file_id, path, A_MESH2, &(meshlink_instance->mesh2)))
    {
        printf("***** ERROR(%s): Cannot read mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_MESH2);
        success = FALSE;
    }

    if (!read_string_attribute(file_id, path, A_TYPE, &type))
    {
        printf("***** ERROR(%s): Cannot read mandatory attribute \"%s@%s\". *****\n\n", C_MESH, path, A_TYPE);
        success = FALSE;
    }

    if (H5Lexists(file_id, path, H5P_DEFAULT) > 0)
        if (H5LTget_dataset_ndims(file_id, path, &nb_dims) >= 0)
            if (nb_dims == 2)
                if (H5LTget_dataset_info(file_id, path, meshlink_instance->dims, &type_class, &length) >= 0)
                    if (type_class == H5T_INTEGER)
                        if (read_int_dataset(file_id, path, meshlink_instance->dims[0] * meshlink_instance->dims[1], &(meshlink_instance->data)))
                            dataset_read = TRUE;
    if (!dataset_read)
    {
        printf("***** ERROR(%s): Cannot read dataset \"%s\". *****\n\n", C_MESH, path);
        meshlink_instance->dims[0] = 0;
        meshlink_instance->dims[1] = 0;
        success = FALSE;
    }

    if (success)
    {
        if (strcmp(type, V_NODE))
            meshlink_instance->type = MSHLNK_NODE;
        else if (strcmp(type, V_EDGE))
            meshlink_instance->type = MSHLNK_EDGE;
        else if (strcmp(type, V_FACE))
            meshlink_instance->type = MSHLNK_FACE;
        else if (strcmp(type, V_VOLUME))
            meshlink_instance->type = MSHLNK_VOLUME;
        else
            meshlink_instance->type = MSHLNK_INVALID;
    }
    else
        meshlink_instance->type = MSHLNK_INVALID;
    if (type != NULL)
    {
        free(type);
        type = NULL;
    }
    free(path2);
}



// Read mesh group
void read_mesh_group (hid_t file_id, const char *path, mesh_group_t *mesh_group)
{
    children_t children;
    hsize_t i, j = 0;
    char *path2, *path3;

    path2 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));
    mesh_group->name = get_name_from_path(path);
    children = read_children_name(file_id, path);
    mesh_group->nb_mesh_instances = children.nb_children;
    for (i = 0; i < children.nb_children; i++)
        if (strcmp(children.childnames[i], G_MESH_LINK) == 0)
            mesh_group->nb_mesh_instances--;    // do not count /meshLink
    mesh_group->mesh_instances = NULL;
    if (children.nb_children > 0)
    {
        mesh_group->mesh_instances = (mesh_instance_t *) malloc((size_t) mesh_group->nb_mesh_instances * sizeof(mesh_instance_t));
        for (i = 0; i < children.nb_children; i++)
        {
            if (strcmp(children.childnames[i], G_MESH_LINK) != 0)
            {
                strcpy(path2, path);
                strcat(path2, children.childnames[i]);
                read_mesh_instance(file_id, path2, mesh_group->mesh_instances + j++);
            }
            free(children.childnames[i]);
        }
        free(children.childnames);
    }

    strcpy(path2, path);
    strcat(path2, G_MESH_LINK);
    children = read_children_name(file_id, path2);
    mesh_group->nb_meshlink_instances = children.nb_children;
    mesh_group->meshlink_instances = NULL;
    if (children.nb_children > 0)
    {
        path3 = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));
        mesh_group->meshlink_instances = (meshlink_instance_t *) malloc((size_t) children.nb_children * sizeof(meshlink_instance_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path3, path2);
            strcat(path3, children.childnames[i]);
            read_meshlink_instance(file_id, path3, mesh_group->meshlink_instances + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
        free(path3);
    }

    free(path2);
}


// Read mesh category
void read_mesh (hid_t file_id, mesh_t *mesh)
{
    hsize_t i;
    children_t children;
    char *path;

    children = read_children_name(file_id, C_MESH);
    mesh->nb_mesh_groups = children.nb_children;
    mesh->mesh_groups = NULL;
    if (children.nb_children > 0)
    {
        path = (char *) malloc(ABSOLUTE_PATH_NAME_LENGTH * sizeof(char));
        mesh->mesh_groups = (mesh_group_t *) malloc((size_t) children.nb_children * sizeof(mesh_group_t));
        for (i = 0; i < children.nb_children; i++)
        {
            strcpy(path, C_MESH);
            strcat(path, children.childnames[i]);
            read_mesh_group(file_id, path, mesh->mesh_groups + i);
            free(children.childnames[i]);
        }
        free(children.childnames);
        free(path);
    }
}




// Print structured mesh
void print_smesh (smesh_t smesh)
{
    hsize_t i;

    printf("       -groups:\n");
    printf("          Number of groups: %lu\n", (long unsigned) smesh.nb_groups);
    for (i = 0; i < smesh.nb_groups; i++)
    {
        printf("           Name: %s\n", smesh.groups[i].name);
        if (smesh.groups[i].type != NULL)
            printf("             @type: %s\n", smesh.groups[i].type);
        if (smesh.groups[i].entitytype != NULL)
            printf("             @entityType: %s\n", smesh.groups[i].entitytype);
        if (smesh.groups[i].normals != NULL)
            printf("             -normals: yes\n");
    }
    printf("       -groupgroups:\n");
    printf("          Number of groupGroups: %lu\n", (unsigned long) smesh.nb_groupgroups);
    for (i = 0; i < smesh.nb_groupgroups; i++)
        printf("           Name: %s\n", smesh.groupgroups[i].name);

    if (smesh.nb_som_tables > 0)
    {
        printf("       -selector on mesh:\n");
        for (i = 0; i < smesh.nb_som_tables; i++)
        {
            printf("             Name: %s\n", smesh.som_tables[i].name);
            printf("               -number of points: %lu\n", (long unsigned) smesh.som_tables[i].nb_points);
        }
    }

}

void print_umesh_som_table (usom_table_t usom_table)
{
    hsize_t k;
    char dim;

    switch (usom_table.type)
    {
    case SOM_POINT_IN_ELEMENT:
        printf("          Name(points): %s\n", usom_table.name);
        for (k = 0; k < usom_table.data.pie.nb_points; k++)
        {
            dim = usom_table.data.pie.nb_dims;
            if (dim == 3)
                if (usom_table.data.pie.vectors[k][2] == -1)
                    dim = 2;
            if (dim == 2)
                if (usom_table.data.pie.vectors[k][1] == -1)
                    dim = 1;

            switch (dim)
            {
            case 1:
                printf("               Point %lu: index=%i, v1=%f\n", (long unsigned) k, usom_table.data.pie.indices[k], usom_table.data.pie.vectors[k][0]);
                break;
            case 2:
                printf("               Point %lu: index=%i, v1=%f, v2=%f\n", (long unsigned) k, usom_table.data.pie.indices[k], usom_table.data.pie.vectors[k][0], usom_table.data.pie.vectors[k][1]);
                break;
            case 3:
                printf("               Point %lu: index=%i, v1=%f, v2=%f, v3=%f\n", (long unsigned) k, usom_table.data.pie.indices[k], usom_table.data.pie.vectors[k][0], usom_table.data.pie.vectors[k][1], usom_table.data.pie.vectors[k][2]);
                break;
            }
        }
        break;
    case SOM_EDGE:
        printf("          Name(edges): %s\n", usom_table.name);
        for (k = 0; k < usom_table.data.ef.nb_items; k++)
        {
            printf("               Id %lu: element=%i, inner_id=%i\n", (long unsigned) k, usom_table.data.ef.items[k][0], usom_table.data.ef.items[k][1]);
        }
        break;
    case SOM_FACE:
        printf("          Name(faces): %s\n", usom_table.name);
        for (k = 0; k < usom_table.data.ef.nb_items; k++)
        {
            printf("               Id %lu: element=%i, inner_id=%i\n", (long unsigned) k, usom_table.data.ef.items[k][0], usom_table.data.ef.items[k][1]);
        }
        break;
    default:
        break;
    }
}


// Print unstructured mesh
void print_umesh (umesh_t umesh)
{
    hsize_t i, offset = 0;
    int j, element_type;

    if (umesh.nb_nodes[0] > 0)
    {
        printf("       -nodes:\n");
        printf("          Number of nodes: %lu\n", (unsigned long) umesh.nb_nodes[0]);
        for (i = 0; i < umesh.nb_nodes[0]; i++)
        {
            printf("           Node n�%lu: ", (unsigned long) i);
            for (j = 0; j < (int) umesh.nb_nodes[1]; j++)
                printf("%f ", umesh.nodes[i][j]);
            printf("\n");
        }
    }
    if (umesh.nb_elementtypes > 0 && umesh.nb_elementnodes > 0)
    {
        printf("       -elements:\n");
        printf("          Number of elements : %lu\n", (unsigned long) umesh.nb_elementtypes);
        for (i = 0; i < umesh.nb_elementtypes; i++)
        {
            printf("           Element %lu: type=%i", (unsigned long) i, umesh.elementtypes[i]);
            element_type = umesh.elementtypes[i];
            if (element_type == 1)
                for (j = 0; j < 2; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 2)
                for (j = 0; j < 3; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 11)
                for (j = 0; j < 3; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 12)
                for (j = 0; j < 6; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 13)
                for (j = 0; j < 4; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 14)
                for (j = 0; j < 8; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 101)
                for (j = 0; j < 4; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 102)
                for (j = 0; j < 5; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 103)
                for (j = 0; j < 6; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            else if (element_type == 104)
                for (j = 0; j < 8; j++)
                    printf(", N%i=%i", j, umesh.elementnodes[offset++]);
            printf("\n");
        }
    }
    printf("       -groups:\n");
    printf("          Number of groups: %lu\n", (unsigned long) umesh.nb_groups);
    for (i = 0; i < umesh.nb_groups; i++)
        printf("           Name: %s\n", umesh.groups[i].name);
    printf("       -groupgroups:\n");
    printf("          Number of groupGroups: %lu\n", (unsigned long) umesh.nb_groupgroups);
    for (i = 0; i < umesh.nb_groupgroups; i++)
        printf("           Name : %s\n", umesh.groupgroups[i].name);

    if (umesh.nb_som_tables > 0)
    {
        printf("       -selector on mesh:\n");
        for (i = 0; i < umesh.nb_som_tables; i++)
        {
            print_umesh_som_table(umesh.som_tables[i]);
        }
    }





}


// Print mesh instance
void print_mesh_instance (mesh_instance_t mesh_instance)
{
    printf("   Mesh instance: %s\n", mesh_instance.name);
    switch (mesh_instance.type)
    {
    case MSH_STRUCTURED:
        print_smesh(mesh_instance.data.structured);
        break;
    case MSH_UNSTRUCTURED:
        print_umesh(mesh_instance.data.unstructured);
        break;
    default:
        break;
    }
}


// Print meshLink instance
void print_meshlink_instance (meshlink_instance_t meshlink_instance)
{
    printf("   MeshLink instance: %s\n", meshlink_instance.name);
    printf("      -@mesh1: %s\n", meshlink_instance.mesh1);
    printf("      -@mesh2: %s\n", meshlink_instance.mesh2);
}


// Print mesh group
void print_mesh_group (mesh_group_t mesh_group)
{
    hsize_t i;

    printf("Mesh group: %s\n", mesh_group.name);
    for (i = 0; i < mesh_group.nb_mesh_instances; i++)
        print_mesh_instance(mesh_group.mesh_instances[i]);
    for (i = 0; i < mesh_group.nb_meshlink_instances; i++)
        print_meshlink_instance(mesh_group.meshlink_instances[i]);
}


// Print mesh category
void print_mesh (mesh_t mesh)
{
    hsize_t i;

    printf("\n###################################  Mesh  ###################################\n\n");
    for (i = 0; i < mesh.nb_mesh_groups; i++)
        print_mesh_group(mesh.mesh_groups[i]);
    printf("\n");
}




// Free memory used by grouGgroup
void free_groupgroup (groupgroup_t *groupgroup)
{
    if (groupgroup->name != NULL)
    {
        free(groupgroup->name);  // free groupGroup name
        groupgroup->name = NULL;
    }

    if (groupgroup->nb_groupgroupnames > 0)  // if groupGroup is not empty...
    {
        free(*(groupgroup->groupgroupnames));  // free groupGroup member names (strings)
        free(groupgroup->groupgroupnames);  // free groupGroup member names
        groupgroup->groupgroupnames = NULL;
        groupgroup->nb_groupgroupnames = 0;
    }
}


// Free memory used by structured mesh
void free_smesh (smesh_t *smesh)
{
    hsize_t i;

    if (smesh->x.nb_nodes > 0)
    {
        free(smesh->x.nodes);
        smesh->x.nodes = NULL;
        smesh->x.nb_nodes = 0;
    }
    if (smesh->y.nb_nodes > 0)
    {
        free(smesh->y.nodes);
        smesh->y.nodes = NULL;
        smesh->y.nb_nodes = 0;
    }
    if (smesh->z.nb_nodes > 0)
    {
        free(smesh->z.nodes);
        smesh->z.nodes = NULL;
        smesh->z.nb_nodes = 0;
    }

    if (smesh->nb_groups > 0)  // if any groups...
    {
        for (i = 0; i < smesh->nb_groups; i++)    // for each group...
        {
            if (smesh->groups[i].nb_elements > 0)  // if group is not empty...
            {
                free(*(smesh->groups[i].elements));  // free group values
                free(smesh->groups[i].elements);
            }

            if (smesh->groups[i].normals != NULL)
            {
                free(*(smesh->groups[i].normals));
                free(smesh->groups[i].normals);
            }

            free(smesh->groups[i].name);  // free group name

            if (smesh->groups[i].type != NULL)
                free(smesh->groups[i].type);  // free group type

            if (smesh->groups[i].entitytype != NULL)
                free(smesh->groups[i].entitytype);  // free group entitytype
        }
        free(smesh->groups);  // free space for pointers to groups
        smesh->groups = NULL;
        smesh->nb_groups = 0;
    }

    if (smesh->nb_groupgroups > 0)  // if any groupGroups...
    {
        for (i = 0; i < smesh->nb_groupgroups; i++)    // for each groupGroup...
            free_groupgroup(smesh->groupgroups + i);  // free groupgroup_t structures
        free(smesh->groupgroups);  // free space for pointers to groupGroups
        smesh->groupgroups = NULL;
        smesh->nb_groupgroups = 0;
    }

    if (smesh->nb_som_tables > 0)
    {
        for (i = 0; i < smesh->nb_som_tables; i++)
        {
            if (smesh->som_tables[i].name != NULL)
                free(smesh->som_tables[i].name);
            if (smesh->som_tables[i].nb_points > 0)
            {
                free(*(smesh->som_tables[i].elements));
                free(smesh->som_tables[i].elements);
                free(*(smesh->som_tables[i].vectors));
                free(smesh->som_tables[i].vectors);
            }

        }
        free(smesh->som_tables);
        smesh->som_tables = NULL;
        smesh->nb_som_tables = 0;
    }
}


// Free memory used by unstructured mesh
void free_umesh (umesh_t *umesh)
{
    hsize_t i;

    if (umesh->nb_elementnodes > 0)  // if any elementnodes...
    {
        free(umesh->elementnodes);
        umesh->elementnodes = NULL;
        umesh->nb_elementnodes = 0;
    }

    if (umesh->nb_elementtypes > 0)  // if any elementtypes...
    {
        free(umesh->elementtypes);
        umesh->elementtypes = NULL;
        umesh->nb_elementtypes = 0;
    }

    if (umesh->nb_nodes[0] > 0)  // if any nodes...
    {
        free(*(umesh->nodes));
        free(umesh->nodes);
        umesh->nodes = NULL;
        umesh->nb_nodes[0] = 0;
        umesh->nb_nodes[1] = 0;
    }

    if (umesh->nb_groups > 0)  // if any groups...
    {
        for (i = 0; i < umesh->nb_groups; i++)    // for each group...
        {
            if (umesh->groups[i].name != NULL)
                free(umesh->groups[i].name);  // free group name
            if (umesh->groups[i].nb_groupelts > 0)  // if group is not empty...
                free(umesh->groups[i].groupelts);  // free group values (no need to assign NULL & set nb_groupelts to 0
        }
        free(umesh->groups);  // free space for pointers to groups
        umesh->groups = NULL;
        umesh->nb_groups = 0;
    }

    if (umesh->nb_groupgroups > 0)  // if any groupGroups...
    {
        for (i = 0; i < umesh->nb_groupgroups; i++)    // for each groupGroup...
            free_groupgroup(umesh->groupgroups + i);  // free groupgroup_t structures
        free(umesh->groupgroups);  // free space for pointers to groupGroups
        umesh->groupgroups = NULL;
        umesh->nb_groupgroups = 0;
    }

    if (umesh->nb_som_tables > 0)
    {
        for (i = 0; i < umesh->nb_som_tables; i++)
        {
            if (umesh->som_tables[i].name != NULL)
                free(umesh->som_tables[i].name);
            switch (umesh->som_tables[i].type)
            {
            case SOM_POINT_IN_ELEMENT:
                if (umesh->som_tables[i].data.pie.nb_points > 0)
                {
                    free(umesh->som_tables[i].data.pie.indices);
                    free(*(umesh->som_tables[i].data.pie.vectors));
                    free(umesh->som_tables[i].data.pie.vectors);
                }
                break;
            case SOM_EDGE:
                if (umesh->som_tables[i].data.ef.nb_items > 0)
                {
                    free(*(umesh->som_tables[i].data.ef.items));
                    free(umesh->som_tables[i].data.ef.items);
                }
                break;
            case SOM_FACE:
                if (umesh->som_tables[i].data.ef.nb_items > 0)
                {
                    free(*(umesh->som_tables[i].data.ef.items));
                    free(umesh->som_tables[i].data.ef.items);
                }
                break;
            default:
                break;
            }
        }
        free(umesh->som_tables);
        umesh->nb_som_tables = 0;
    }
}


// Free memory used by mesh instance
void free_mesh_instance (mesh_instance_t *mesh_instance)
{
    if (mesh_instance->name != NULL)
    {
        free(mesh_instance->name);
        mesh_instance->name = NULL;
    }

    switch (mesh_instance->type)
    {
    case MSH_STRUCTURED:
        free_smesh(&(mesh_instance->data.structured));
        break;
    case MSH_UNSTRUCTURED:
        free_umesh(&(mesh_instance->data.unstructured));
        break;
    default:
        break;
    }
}


void free_meshlink_instance (meshlink_instance_t *meshlink_instance)
{
    if (meshlink_instance->name != NULL)
    {
        free(meshlink_instance->name);
        meshlink_instance->name = NULL;
    }
    meshlink_instance->type = MSHLNK_INVALID;
    if (meshlink_instance->mesh1 != NULL)
    {
        free(meshlink_instance->mesh1);
        meshlink_instance->mesh1 = NULL;
    }
    if (meshlink_instance->mesh2 != NULL)
    {
        free(meshlink_instance->mesh2);
        meshlink_instance->mesh2 = NULL;
    }
    if (meshlink_instance->data != NULL)
    {
        free(meshlink_instance->data);
        meshlink_instance->data = NULL;
        meshlink_instance->dims[0] = 0;
        meshlink_instance->dims[1] = 0;
    }
}


// Free memory used by mesh group
void free_mesh_group (mesh_group_t *mesh_group)
{
    hsize_t i;

    if (mesh_group->name != NULL)
    {
        free(mesh_group->name);
        mesh_group->name = NULL;
    }

    if (mesh_group->nb_mesh_instances > 0)
    {
        for (i = 0; i < mesh_group->nb_mesh_instances; i++)
            free_mesh_instance(mesh_group->mesh_instances + i);
        free(mesh_group->mesh_instances);
        mesh_group->mesh_instances = NULL;
        mesh_group->nb_mesh_instances = 0;
    }
    if (mesh_group->nb_meshlink_instances > 0)
    {
        for (i = 0; i < mesh_group->nb_meshlink_instances; i++)
            free_meshlink_instance(mesh_group->meshlink_instances + i);
        free(mesh_group->meshlink_instances);
        mesh_group->meshlink_instances = NULL;
        mesh_group->nb_meshlink_instances = 0;
    }
}


// Free memory used by mesh category
void free_mesh (mesh_t *mesh)
{
    hsize_t i;

    if (mesh->nb_mesh_groups > 0)
    {
        for (i = 0; i < mesh->nb_mesh_groups; i++)
            free_mesh_group(mesh->mesh_groups + i);
        free(mesh->mesh_groups);
        mesh->mesh_groups = NULL;
        mesh->nb_mesh_groups = 0;
    }
}

