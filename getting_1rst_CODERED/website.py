from flask import Flask, render_template, request, redirect, url_for, session, flash
import csv
from datetime import datetime

app = Flask(__name__)
app.secret_key = 'hackathon_secret_key'

# CSV file for storing user data
USER_DATA_FILE = 'users.csv'
FORM_DATA_FILE = 'data.csv'

# Admin credentials
ADMIN_USERNAME = 'SSSS'
ADMIN_PASSWORD = 'SSSS'


# Utility function to check if user exists
def user_exists(username):
    try:
        with open(USER_DATA_FILE, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                if row['username'] == username:
                    return True
    except FileNotFoundError:
        return False
    return False


def validate_user(username, password):
    # Check for admin credentials
    if username == ADMIN_USERNAME and password == ADMIN_PASSWORD:
        return 'admin'

    # Check for client credentials
    try:
        with open(USER_DATA_FILE, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                if row['username'] == username and row['password'] == password:
                    return 'client'
    except FileNotFoundError:
        return None
    return None



@app.route('/')
def index():
    return render_template('index.html', username=session.get('username'))


# Route: Signup
@app.route('/signup', methods=['GET', 'POST'])
def signup():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']

        # Check if the user already exists
        if user_exists(username):
            flash('Username already exists. Please log in.', 'error')
            return redirect(url_for('login'))

        # Add the new user to the CSV file
        with open(USER_DATA_FILE, 'a', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=['username', 'password'])
            if csvfile.tell() == 0:  # Write header if file is empty
                writer.writeheader()
            writer.writerow({'username': username, 'password': password})

        flash('Signup successful! Please log in.', 'success')
        return redirect(url_for('login'))

    return render_template('signup.html')


@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']

        # Validate user and determine role
        role = validate_user(username, password)
        if role:
            session['username'] = username
            session['role'] = role

            # Redirect based on role
            if role == 'admin':
                return redirect(url_for('admin_dashboard'))
            else:
                return redirect(url_for('client_dashboard'))
        else:
            flash('Invalid username or password.', 'error')

    return render_template('login.html')



# Route: Logout
@app.route('/logout')
def logout():
    session.clear()
    flash('You have been logged out.', 'info')
    return redirect(url_for('login'))


# Route: Admin Dashboard
@app.route('/admin')
def admin_dashboard():
    if 'role' not in session or session['role'] != 'admin':
        return redirect(url_for('login'))

    # Read data from CSV
    try:
        with open(FORM_DATA_FILE, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            data = list(reader)
    except FileNotFoundError:
        data = []

    return render_template('admin_dashboard.html', data=data)


# Route: Client Dashboard
@app.route('/client')
def client_dashboard():
    if 'role' not in session or session['role'] != 'client':
        return redirect(url_for('login'))

    return render_template('client_dashboard.html', username=session['username'])

@app.route('/update_status', methods=['POST'])
def update_status():
    if 'role' not in session or session['role'] != 'admin':
        return redirect(url_for('login'))

    vehicle_number = request.form['vehicle_number']
    action = request.form['action']

    # Update the product status in the CSV file
    updated_data = []
    try:
        with open(FORM_DATA_FILE, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                if row['Vehicle Number'] == vehicle_number:
                    if action == 'approve':
                        row['Product Status'] = 'YES'
                    elif action == 'deny':
                        row['Product Status'] = 'NULL'
                updated_data.append(row)
    except FileNotFoundError:
        flash('No data file found.', 'error')
        return redirect(url_for('admin_dashboard'))

    # Write updated data back to the CSV file
    with open(FORM_DATA_FILE, 'w', newline='') as csvfile:
        # Include the "Type" field in fieldnames
        fieldnames = ['Vehicle Number', 'PAN', 'Aadhar', 'Driver\'s License', 'Timestamp', 'Type', 'Product Status']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()  # Write the header
        writer.writerows(updated_data)  # Write the updated rows

    flash('Status updated successfully!', 'success')
    return redirect(url_for('admin_dashboard'))

# !!!!!
@app.route('/project_dets')
def project_details():
    return render_template('project_dets.html')

# submitted 
@app.route('/submit', methods=['POST'])
def submit():
    if 'role' not in session or session['role'] != 'client':
        return redirect(url_for('login'))

    # Retrieve form data
    vehicle_number = request.form['vehicle_number']
    pan = request.form['pan']
    aadhar = request.form['aadhar']
    license = request.form['license']
    vehicle_type = request.form['vehicle_type']
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    product_status = "NO"

    # Check if the vehicle number already exists
    try:
        with open(FORM_DATA_FILE, 'r') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                if row['Vehicle Number'] == vehicle_number:
                    return render_template(
                        'client_dashboard.html',
                        username=session['username'],
                        success=False,
                        already_registered=True
                    )
    except FileNotFoundError:
        pass

    # Save new data to CSV
    with open(FORM_DATA_FILE, 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        if csvfile.tell() == 0:  # Write header if file is empty
            writer.writeheader(['Vehicle Number', 'PAN', 'Aadhar', 'Driver\'s License', 'Type','Timestamp', 'Product Status'])
        writer.writerow([vehicle_number, pan, aadhar, license, vehicle_type, timestamp, product_status])

    return render_template(
        'client_dashboard.html',
        username=session['username'],
        success=True,
        already_registered=False
    )


if __name__ == '__main__':
    app.run(debug=True)
