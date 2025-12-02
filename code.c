/*
  bus_manager.c
  Combined single-file version of "College Bus Fee & Route Management"
  Contains: Route management, Student management, Billing, File I/O, Menu
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
#endif

/* ----------------------------- Config ----------------------------- */
#define MAX_ROUTE_NAME 64
#define MAX_NAME 64

#define ROUTES_FILE "data/routes.dat"
#define STUDENTS_FILE "data/students.dat"
#define RECEIPTS_FILE "data/receipts.txt"

/* ----------------------------- Types ------------------------------ */

typedef struct {
    int id;
    char name[MAX_ROUTE_NAME];
    double distance_km;
    double rate_per_km;
} Route;

typedef struct {
    Route *arr;
    int size;
    int cap;
} RouteList;

typedef struct {
    int id;
    char name[MAX_NAME];
    int routeId;
} Student;

typedef struct {
    Student *arr;
    int size;
    int cap;
} StudentList;

/* --------------------------- Globals ------------------------------ */

static int nextRouteId = 1;
static int nextStudentId = 1;

/* ----------------------- RouteList functions ---------------------- */

void initRouteList(RouteList *rl) {
    rl->arr = NULL;
    rl->size = 0;
    rl->cap = 0;
}

void freeRouteList(RouteList *rl) {
    if (rl->arr) free(rl->arr);
    rl->arr = NULL;
    rl->size = rl->cap = 0;
}

static void ensureRouteCap(RouteList *rl) {
    if (rl->size >= rl->cap) {
        int newCap = rl->cap == 0 ? 4 : rl->cap * 2;
        Route *tmp = (Route*)realloc(rl->arr, sizeof(Route) * newCap);
        if (!tmp) return;
        rl->arr = tmp;
        rl->cap = newCap;
    }
}

int addRoute(RouteList *rl, const char *name, double distance_km, double rate_per_km) {
    ensureRouteCap(rl);
    if (rl->size >= rl->cap) return -1;
    Route *r = &rl->arr[rl->size];
    r->id = nextRouteId++;
    strncpy(r->name, name, MAX_ROUTE_NAME-1);
    r->name[MAX_ROUTE_NAME-1] = '\0';
    r->distance_km = distance_km;
    r->rate_per_km = rate_per_km;
    rl->size++;
    return r->id;
}

int findRouteIndexById(RouteList *rl, int id) {
    for (int i = 0; i < rl->size; ++i) if (rl->arr[i].id == id) return i;
    return -1;
}

Route *getRouteById(RouteList *rl, int id) {
    int idx = findRouteIndexById(rl, id);
    if (idx < 0) return NULL;
    return &rl->arr[idx];
}

void removeRouteById(RouteList *rl, int id) {
    int idx = findRouteIndexById(rl, id);
    if (idx < 0) return;
    for (int i = idx; i < rl->size - 1; ++i) rl->arr[i] = rl->arr[i+1];
    rl->size--;
}

void printAllRoutes(RouteList *rl) {
    printf("\nAvailable routes:\n");
    if (rl->size == 0) { printf("(none)\n"); return; }
    for (int i = 0; i < rl->size; ++i) {
        Route *r = &rl->arr[i];
        printf("ID: %d | %s | Distance: %.2f km | Rate: %.2f per km\n",
               r->id, r->name, r->distance_km, r->rate_per_km);
    }
}

/* ---------------------- StudentList functions --------------------- */

void initStudentList(StudentList *sl) {
    sl->arr = NULL;
    sl->size = 0;
    sl->cap = 0;
}

void freeStudentList(StudentList *sl) {
    if (sl->arr) free(sl->arr);
    sl->arr = NULL;
    sl->size = sl->cap = 0;
}

static void ensureStudentCap(StudentList *sl) {
    if (sl->size >= sl->cap) {
        int newCap = sl->cap == 0 ? 8 : sl->cap * 2;
        Student *tmp = (Student*)realloc(sl->arr, sizeof(Student) * newCap);
        if (!tmp) return;
        sl->arr = tmp;
        sl->cap = newCap;
    }
}

int addStudent(StudentList *sl, const char *name, int routeId) {
    ensureStudentCap(sl);
    if (sl->size >= sl->cap) return -1;
    Student *s = &sl->arr[sl->size];
    s->id = nextStudentId++;
    strncpy(s->name, name, MAX_NAME-1);
    s->name[MAX_NAME-1] = '\0';
    s->routeId = routeId;
    sl->size++;
    return s->id;
}

int findStudentIndexById(StudentList *sl, int id) {
    for (int i = 0; i < sl->size; ++i) if (sl->arr[i].id == id) return i;
    return -1;
}

Student *getStudentById(StudentList *sl, int id) {
    int idx = findStudentIndexById(sl, id);
    if (idx < 0) return NULL;
    return &sl->arr[idx];
}

void removeStudentById(StudentList *sl, int id) {
    int idx = findStudentIndexById(sl, id);
    if (idx < 0) return;
    for (int i = idx; i < sl->size - 1; ++i) sl->arr[i] = sl->arr[i+1];
    sl->size--;
}

void printAllStudents(StudentList *sl) {
    printf("\nStudents:\n");
    if (sl->size == 0) { printf("(none)\n"); return; }
    for (int i = 0; i < sl->size; ++i) {
        Student *s = &sl->arr[i];
        printf("ID: %d | %s | Route ID: %d\n", s->id, s->name, s->routeId);
    }
}

/* --------------------------- Billing ------------------------------ */

double calculateFeeForStudent(Student *s, RouteList *rl) {
    if (!s) return 0.0;
    Route *r = getRouteById(rl, s->routeId);
    if (!r) return 0.0;
    double baseFare = 50.0; // example base fare
    double fee = baseFare + (r->distance_km * r->rate_per_km);
    if (r->distance_km < 5.0) fee *= 0.95; // small discount
    return fee;
}

int generateFeeSlip(Student *s, RouteList *rl, const char *filename) {
    if (!s) return -1;
    Route *r = getRouteById(rl, s->routeId);
    if (!r) return -1;
    double amount = calculateFeeForStudent(s, rl);
    FILE *f = fopen(filename, "a");
    if (!f) return -1;
    time_t t = time(NULL);
    fprintf(f, "-------------------------------\n");
    fprintf(f, "Date: %s", ctime(&t));
    fprintf(f, "Student ID: %d\nName: %s\nRoute: %s (ID %d)\n", s->id, s->name, r->name, r->id);
    fprintf(f, "Distance: %.2f km | Rate: %.2f | Amount: %.2f\n", r->distance_km, r->rate_per_km, amount);
    fprintf(f, "-------------------------------\n\n");
    fclose(f);
    return 0;
}

void printSummary(StudentList *sl, RouteList *rl) {
    printf("\nSummary report:\n");
    if (rl->size == 0) { printf("No routes.\n"); return; }
    for (int i = 0; i < rl->size; ++i) {
        Route *r = &rl->arr[i];
        int count = 0; double total = 0.0;
        for (int j = 0; j < sl->size; ++j) {
            Student *s = &sl->arr[j];
            if (s->routeId == r->id) {
                count++;
                total += calculateFeeForStudent(s, rl);
            }
        }
        printf("Route %s (ID %d): %d students | Revenue: %.2f\n", r->name, r->id, count, total);
    }
}

/* --------------------------- File I/O ----------------------------- */

int ensureDataFolder() {
#ifdef _WIN32
    _mkdir("data");
    return 0;
#else
    struct stat st = {0};
    if (stat("data", &st) == -1) {
        if (mkdir("data", 0755) == 0) return 0;
        else return -1;
    }
    return 0;
#endif
}

int saveRoutesToFile(RouteList *rl, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    fwrite(&rl->size, sizeof(int), 1, f);
    if (rl->size > 0) fwrite(rl->arr, sizeof(Route), rl->size, f);
    fclose(f);
    return 0;
}

int loadRoutesFromFile(RouteList *rl, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    int n = 0;
    if (fread(&n, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    freeRouteList(rl);
    if (n > 0) {
        rl->arr = (Route*)malloc(sizeof(Route) * n);
        if (!rl->arr) { fclose(f); return -1; }
        if (fread(rl->arr, sizeof(Route), n, f) != (size_t)n) { free(rl->arr); rl->arr = NULL; fclose(f); return -1; }
        rl->size = n; rl->cap = n;
        int maxid = 0;
        for (int i = 0; i < rl->size; ++i) if (rl->arr[i].id > maxid) maxid = rl->arr[i].id;
        nextRouteId = maxid + 1;
    }
    fclose(f);
    return 0;
}

int saveStudentsToFile(StudentList *sl, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    fwrite(&sl->size, sizeof(int), 1, f);
    if (sl->size > 0) fwrite(sl->arr, sizeof(Student), sl->size, f);
    fclose(f);
    return 0;
}

int loadStudentsFromFile(StudentList *sl, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    int n = 0;
    if (fread(&n, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    freeStudentList(sl);
    if (n > 0) {
        sl->arr = (Student*)malloc(sizeof(Student) * n);
        if (!sl->arr) { fclose(f); return -1; }
        if (fread(sl->arr, sizeof(Student), n, f) != (size_t)n) { free(sl->arr); sl->arr = NULL; fclose(f); return -1; }
        sl->size = n; sl->cap = n;
        int maxid = 0;
        for (int i = 0; i < sl->size; ++i) if (sl->arr[i].id > maxid) maxid = sl->arr[i].id;
        nextStudentId = maxid + 1;
    }
    fclose(f);
    return 0;
}

/* ----------------------------- Utilities -------------------------- */

void pause_console() { printf("\nPress Enter to continue..."); getchar(); }

void seedExampleData(RouteList *rl, StudentList *sl) {
    if (rl->size == 0) {
        addRoute(rl, "North Campus", 4.5, 6.0);
        addRoute(rl, "East Colony", 12.0, 5.0);
        addRoute(rl, "West Market", 8.0, 5.5);
    }
    if (sl->size == 0) {
        addStudent(sl, "Aman Kumar", rl->arr[0].id);
        addStudent(sl, "Priya Singh", rl->arr[1].id);
    }
}

/* ------------------------------- Main ----------------------------- */

int main(void) {
    RouteList routes;
    StudentList students;
    initRouteList(&routes);
    initStudentList(&students);

    ensureDataFolder();
    loadRoutesFromFile(&routes, ROUTES_FILE);
    loadStudentsFromFile(&students, STUDENTS_FILE);
    seedExampleData(&routes, &students);

    int choice = -1;
    while (1) {
        printf("\n===== College Bus Fee & Route Manager =====\n");
        printf("1. Show routes\n2. Add route\n3. Remove route\n");
        printf("4. Show students\n5. Add student\n6. Remove student\n");
        printf("7. Generate fee slip for student\n8. Print summary\n");
        printf("9. Save data\n0. Exit\n");
        printf("Choose: ");
        if (scanf("%d", &choice) != 1) { while (getchar()!='\n'); choice = -1; }
        getchar(); // consume newline

        if (choice == 1) {
            printAllRoutes(&routes);
            pause_console();
        }
        else if (choice == 2) {
            char name[ MAX_ROUTE_NAME ];
            double dist = 0.0, rate = 0.0;
            printf("Route name: ");
            fgets(name, sizeof(name), stdin); name[strcspn(name, "\n")] = '\0';
            printf("Distance (km): "); scanf("%lf", &dist); getchar();
            printf("Rate per km: "); scanf("%lf", &rate); getchar();
            int id = addRoute(&routes, name, dist, rate);
            printf("Added route with ID %d\n", id);
            pause_console();
        }
        else if (choice == 3) {
            int id;
            printf("Route ID to remove: "); scanf("%d", &id); getchar();
            removeRouteById(&routes, id);
            printf("Removed if existed.\n");
            pause_console();
        }
        else if (choice == 4) {
            printAllStudents(&students);
            pause_console();
        }
        else if (choice == 5) {
            char name[MAX_NAME];
            int routeid;
            printf("Student name: "); fgets(name, sizeof(name), stdin); name[strcspn(name, "\n")] = '\0';
            printAllRoutes(&routes);
            printf("Assign route ID (0 for none): "); scanf("%d", &routeid); getchar();
            int sid = addStudent(&students, name, routeid);
            printf("Added student with ID %d\n", sid);
            pause_console();
        }
        else if (choice == 6) {
            int id; printf("Student ID to remove: "); scanf("%d", &id); getchar();
            removeStudentById(&students, id); printf("Removed if existed.\n"); pause_console();
        }
        else if (choice == 7) {
            int id; printf("Student ID: "); scanf("%d", &id); getchar();
            Student *s = getStudentById(&students, id);
            if (!s) { printf("Student not found.\n"); pause_console(); }
            else {
                double amt = calculateFeeForStudent(s, &routes);
                printf("Fee for %s (ID %d): %.2f\n", s->name, s->id, amt);
                if (generateFeeSlip(s, &routes, RECEIPTS_FILE) == 0)
                    printf("Fee slip appended to %s\n", RECEIPTS_FILE);
                else
                    printf("Failed to write fee slip.\n");
                pause_console();
            }
        }
        else if (choice == 8) { printSummary(&students, &routes); pause_console(); }
        else if (choice == 9) {
            if (saveRoutesToFile(&routes, ROUTES_FILE) == 0 && saveStudentsToFile(&students, STUDENTS_FILE) == 0)
                printf("Saved.\n");
            else
                printf("Save failed.\n");
            pause_console();
        }
        else if (choice == 0) {
            saveRoutesToFile(&routes, ROUTES_FILE);
            saveStudentsToFile(&students, STUDENTS_FILE);
            break;
        }
        else {
            printf("Invalid input.\n");
            pause_console();
        }
    }

    freeRouteList(&routes);
    freeStudentList(&students);
    printf("Goodbye.\n");
    return 0;
}