#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/file.h>

#define CUSTOMER 1
#define EMPLOYEE 2
#define MANAGER 3
#define ADMIN 4

// Struct for customer and employee accounts
typedef struct {
    int id;
    char name[50];
    char password[20];
    float balance;
    int status; // 1 for active, 0 for inactive
} CustomerAccount;

typedef struct {
    int id;
    char name[50];
    char password[20];
} EmployeeAccount;

// Function prototypes
void customerMenu(int customerID);
void employeeMenu(int employeeID);
void managerMenu();
void adminMenu();
void depositMoney(int customerID);
void withdrawMoney(int customerID);
void transferMoney(int customerID);
void applyLoan(int customerID);
void changePassword(int customerID);
void viewTransactionHistory(int customerID);
void handleFileLocking(int fd, struct flock *lock, int lockType);
int findCustomerByID(int id, CustomerAccount *account);
void saveCustomer(CustomerAccount *account);
void addNewCustomer();
void modifyCustomerDetails();
void closeCustomerAccount();
void processLoanApplications();
void assignLoanApplications();
void viewCustomerTransactions(int customerID);
void addNewEmployee();
void modifyEmployeeDetails();
void activateDeactivateCustomer(int managerID);

// Global file paths
const char *accountFilePath = "accounts.dat";
const char *employeeFilePath = "employees.dat";
const char *transactionFilePath = "transactions.dat";
const char *loanFilePath = "loans.dat";

// Locking mechanism
void handleFileLocking(int fd, struct flock *lock, int lockType) {
    lock->l_type = lockType;  // F_RDLCK for read, F_WRLCK for write, F_UNLCK for unlock
    lock->l_whence = SEEK_SET;
    lock->l_start = 0;
    lock->l_len = 0;  // Lock the entire file
    
    if (fcntl(fd, F_SETLKW, lock) == -1) {
        perror("File locking failed");
        exit(1);
    }
}

// Utility to find customer by ID
int findCustomerByID(int id, CustomerAccount *account) {
    FILE *file = fopen(accountFilePath, "rb");
    if (!file) {
        perror("File opening error");
        return 0;
    }
    while (fread(account, sizeof(CustomerAccount), 1, file)) {
        if (account->id == id && account->status == 1) {  // Only active accounts
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

// Save updated customer back to file
void saveCustomer(CustomerAccount *account) {
    FILE *file = fopen(accountFilePath, "r+b");
    CustomerAccount temp;
    while (fread(&temp, sizeof(CustomerAccount), 1, file)) {
        if (temp.id == account->id) {
            fseek(file, -sizeof(CustomerAccount), SEEK_CUR);
            fwrite(account, sizeof(CustomerAccount), 1, file);
            break;
        }
    }
    fclose(file);
}

// CUSTOMER FUNCTIONALITIES

void customerMenu(int customerID) {
    int choice;
    while(1) {
        printf("\nCustomer Menu:\n1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Apply for Loan\n6. Change Password\n7. View Transaction History\n8. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: {
                CustomerAccount account;
                if (findCustomerByID(customerID, &account)) {
                    printf("Account balance: %.2f\n", account.balance);
                } else {
                    printf("Account not found.\n");
                }
                break;
            }
            case 2:
                depositMoney(customerID);
                break;
            case 3:
                withdrawMoney(customerID);
                break;
            case 4:
                transferMoney(customerID);
                break;
            case 5:
                applyLoan(customerID);
                break;
            case 6:
                changePassword(customerID);
                break;
            case 7:
                viewTransactionHistory(customerID);
                break;
            case 8:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}

// Deposit money
void depositMoney(int customerID) {
    float amount;
    printf("Enter amount to deposit: ");
    scanf("%f", &amount);

    int fd = open(accountFilePath, O_RDWR);
    if (fd == -1) {
        perror("Unable to open file");
        return;
    }

    struct flock lock;
    handleFileLocking(fd, &lock, F_WRLCK);  // Lock file for writing

    CustomerAccount account;
    if (findCustomerByID(customerID, &account)) {
        account.balance += amount;
        printf("Deposit successful! New balance: %.2f\n", account.balance);
        saveCustomer(&account);
    } else {
        printf("Account not found.\n");
    }

    handleFileLocking(fd, &lock, F_UNLCK);  // Unlock file
    close(fd);
}

// Withdraw money
void withdrawMoney(int customerID) {
    float amount;
    printf("Enter amount to withdraw: ");
    scanf("%f", &amount);

    int fd = open(accountFilePath, O_RDWR);
    if (fd == -1) {
        perror("Unable to open file");
        return;
    }

    struct flock lock;
    handleFileLocking(fd, &lock, F_WRLCK);  // Lock file for writing

    CustomerAccount account;
    if (findCustomerByID(customerID, &account)) {
        if (account.balance >= amount) {
            account.balance -= amount;
            printf("Withdrawal successful! New balance: %.2f\n", account.balance);
            saveCustomer(&account);
        } else {
            printf("Insufficient funds.\n");
        }
    } else {
        printf("Account not found.\n");
    }

    handleFileLocking(fd, &lock, F_UNLCK);  // Unlock file
    close(fd);
}

// Transfer money
void transferMoney(int customerID) {
    int receiverID;
    float amount;
    printf("Enter recipient's account ID: ");
    scanf("%d", &receiverID);
    printf("Enter amount to transfer: ");
    scanf("%f", &amount);

    int fd = open(accountFilePath, O_RDWR);
    if (fd == -1) {
        perror("Unable to open file");
        return;
    }

    struct flock lock;
    handleFileLocking(fd, &lock, F_WRLCK);  // Lock file for writing

    CustomerAccount sender, receiver;
    if (findCustomerByID(customerID, &sender) && findCustomerByID(receiverID, &receiver)) {
        if (sender.balance >= amount) {
            sender.balance -= amount;
            receiver.balance += amount;
            printf("Transfer successful! New balance: %.2f\n", sender.balance);
            saveCustomer(&sender);
            saveCustomer(&receiver);
        } else {
            printf("Insufficient funds.\n");
        }
    } else {
        printf("Account not found.\n");
    }

    handleFileLocking(fd, &lock, F_UNLCK);  // Unlock file
    close(fd);
}

// Apply for a loan
void applyLoan(int customerID) {
    float loanAmount;
    printf("Enter loan amount: ");
    scanf("%f", &loanAmount);

    printf("Loan application for %.2f has been submitted!\n", loanAmount);
    // Loan handling logic should be added here (file saving)
}

// Change password
void changePassword(int customerID) {
    char newPassword[20];
    printf("Enter your new password: ");
    scanf("%s", newPassword);

    CustomerAccount account;
    if (findCustomerByID(customerID, &account)) {
        strcpy(account.password, newPassword);
        saveCustomer(&account);
        printf("Password changed successfully!\n");
    } else {
        printf("Account not found.\n");
    }
}

// View transaction history
void viewTransactionHistory(int customerID) {
    FILE *file = fopen(transactionFilePath, "rb");
    if (!file) {
        perror("File opening error");
        return;
    }
    printf("Transaction history for customer %d:\n", customerID);
    // Logic for displaying customer transaction history
    fclose(file);
}

// EMPLOYEE FUNCTIONALITIES

void employeeMenu(int employeeID) {
    int choice;
    while(1) {
        printf("\nEmployee Menu:\n1. Add New Customer\n2. Modify Customer\n3. Close Customer Account\n4. Process Loans\n5. View Customer Transactions\n6. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                addNewCustomer();
                break;
            case 2:
                modifyCustomerDetails();
                break;
            case 3:
                closeCustomerAccount();
                break;
            case 4:
                processLoanApplications();
                break;
            case 5:
                viewCustomerTransactions(employeeID);
                break;
            case 6:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}

// Add new customer
void addNewCustomer() {
    CustomerAccount newCustomer;
    printf("Enter customer ID: ");
    scanf("%d", &newCustomer.id);
    printf("Enter customer name: ");
    scanf("%s", newCustomer.name);
    printf("Enter initial deposit: ");
    scanf("%f", &newCustomer.balance);
    newCustomer.status = 1;  // Set active

    FILE *file = fopen(accountFilePath, "ab");
    if (!file) {
        perror("File opening error");
        return;
    }
    fwrite(&newCustomer, sizeof(CustomerAccount), 1, file);
    fclose(file);

    printf("New customer added successfully!\n");
}

// Modify customer details
void modifyCustomerDetails() {
    int customerID;
    printf("Enter customer ID to modify: ");
    scanf("%d", &customerID);

    CustomerAccount account;
    if (findCustomerByID(customerID, &account)) {
        printf("Enter new customer name: ");
        scanf("%s", account.name);
        saveCustomer(&account);
        printf("Customer details updated successfully!\n");
    } else {
        printf("Customer not found.\n");
    }
}

// Close customer account
void closeCustomerAccount() {
    int customerID;
    printf("Enter customer ID to close: ");
    scanf("%d", &customerID);

    CustomerAccount account;
    if (findCustomerByID(customerID, &account)) {
        account.status = 0;  // Deactivate account
        saveCustomer(&account);
        printf("Customer account closed successfully!\n");
    } else {
        printf("Customer not found.\n");
    }
}

// Process loan applications
void processLoanApplications() {
    printf("Loan processing not fully implemented yet.\n");
}

// View customer transactions (passbook)
void viewCustomerTransactions(int employeeID) {
    printf("Viewing customer transactions (for passbook feature).\n");
}

// MANAGER FUNCTIONALITIES

void managerMenu() {
    int choice;
    while(1) {
        printf("\nManager Menu:\n1. Activate/Deactivate Customer Account\n2. Assign Loan Applications\n3. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                activateDeactivateCustomer(0); // Manager ID passed for logging
                break;
            case 2:
                assignLoanApplications();
                break;
            case 3:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}

// Activate or deactivate customer account
void activateDeactivateCustomer(int managerID) {
    int customerID, action;
    printf("Enter customer ID: ");
    scanf("%d", &customerID);
    printf("Enter 1 to activate, 0 to deactivate: ");
    scanf("%d", &action);

    CustomerAccount account;
    if (findCustomerByID(customerID, &account)) {
        account.status = action;
        saveCustomer(&account);
        printf("Customer account updated successfully!\n");
    } else {
        printf("Customer not found.\n");
    }
}

// Assign loan applications to employees
void assignLoanApplications() {
    printf("Loan assignment to employees not implemented yet.\n");
}

// ADMIN FUNCTIONALITIES

void adminMenu() {
    int choice;
    while(1) {
        printf("\nAdmin Menu:\n1. Add New Employee\n2. Modify Employee\n3. Logout\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                addNewEmployee();
                break;
            case 2:
                modifyEmployeeDetails();
                break;
            case 3:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}

// Add new employee
void addNewEmployee() {
    EmployeeAccount newEmployee;
    printf("Enter employee ID: ");
    scanf("%d", &newEmployee.id);
    printf("Enter employee name: ");
    scanf("%s", newEmployee.name);
    printf("Enter password: ");
    scanf("%s", newEmployee.password);

    FILE *file = fopen(employeeFilePath, "ab");
    if (!file) {
        perror("File opening error");
        return;
    }
    fwrite(&newEmployee, sizeof(EmployeeAccount), 1, file);
    fclose(file);

    printf("New employee added successfully!\n");
}

// Modify employee details
void modifyEmployeeDetails() {
    int employeeID;
    printf("Enter employee ID to modify: ");
    scanf("%d", &employeeID);

    EmployeeAccount employee;
    FILE *file = fopen(employeeFilePath, "r+b");
    if (!file) {
        perror("File opening error");
        return;
    }

    while (fread(&employee, sizeof(EmployeeAccount), 1, file)) {
        if (employee.id == employeeID) {
            printf("Enter new employee name: ");
            scanf("%s", employee.name);
            fseek(file, -sizeof(EmployeeAccount), SEEK_CUR);
            fwrite(&employee, sizeof(EmployeeAccount), 1, file);
            break;
        }
    }
    fclose(file);
    printf("Employee details updated successfully!\n");
}

// MAIN MENU

int main() {
    int role, userID;

    printf("Select your role:\n1. Customer\n2. Employee\n3. Manager\n4. Admin\n");
    scanf("%d", &role);

    switch (role) {
        case CUSTOMER:
            printf("Enter your customer ID: ");
            scanf("%d", &userID);
            customerMenu(userID);
            break;
        case EMPLOYEE:
            printf("Enter your employee ID: ");
            scanf("%d", &userID);
            employeeMenu(userID);
            break;
        case MANAGER:
            managerMenu();
            break;
        case ADMIN:
            adminMenu();
            break;
        default:
            printf("Invalid role selected.\n");
    }
    
    return 0;
}
