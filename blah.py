import matplotlib.pyplot as plt

# Define the job details
jobs = [
    {"arrival_time": 0, "service_time": 3, "job": "Job 1"},
    {"arrival_time": 3, "service_time": 50, "job": "Job 2"},
    {"arrival_time": 7, "service_time": 2, "job": "Job 3"},
    {"arrival_time": 10, "service_time": 6, "job": "Job 4"},
    {"arrival_time": 10, "service_time": 12, "job": "Job 5"},
    {"arrival_time": 10, "service_time": 4, "job": "Job 6"},
    {"arrival_time": 23, "service_time": 6, "job": "Job 7"},
    {"arrival_time": 27, "service_time": 11, "job": "Job 8"},
    {"arrival_time": 33, "service_time": 3, "job": "Job 9"},
    {"arrival_time": 36, "service_time": 2, "job": "Job 10"},
    {"arrival_time": 42, "service_time": 4, "job": "Job 11"},
    {"arrival_time": 50, "service_time": 6, "job": "Job 12"},
    {"arrival_time": 50, "service_time": 5, "job": "Job 13"},
    {"arrival_time": 50, "service_time": 6, "job": "Job 14"},
    {"arrival_time": 50, "service_time": 1, "job": "Job 15"},
    {"arrival_time": 66, "service_time": 41, "job": "Job 16"},
    {"arrival_time": 71, "service_time": 3, "job": "Job 17"},
    {"arrival_time": 74, "service_time": 2, "job": "Job 18"},
    {"arrival_time": 77, "service_time": 6, "job": "Job 19"}
]

# Plotting
fig, ax = plt.subplots(figsize=(12, 8))

for job in jobs:
    ax.barh(job["job"], job["service_time"], left=job["arrival_time"], color="skyblue", edgecolor="black")

# Labels and title
ax.set_xlabel("Time")
ax.set_ylabel("Jobs")
ax.set_title("Job Service Times with Arrival Times")
plt.grid(axis="x", linestyle="--", alpha=0.7)
plt.show()
