import numpy as np
import torch
import torch.nn as nn
import matplotlib.pyplot as plt
from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay
from sklearn.model_selection import train_test_split
import os
import pandas as pd
from pathlib import Path
import time

np.random.seed(42)
torch.manual_seed(42)

# Data loading
def load_workout_data(data_dir="TrainingDataEAI", window_sec=5, sample_rate=100):
    data_path = Path(data_dir)
    sequences = []
    labels = []

    activities = [
        ("WeightLift_", 0),
        ("Walking_", 1),
        ("Plank_", 2),
        ("JumpingJacks_", 3),
        ("Squats_", 4),
        ("JumpRope_", 5)
    ]

    window_size = int(window_sec * sample_rate)

    for prefix, label in activities:
        dirs = sorted([d for d in data_path.iterdir()
                      if d.is_dir() and d.name.startswith(prefix)])

        print(f"Found {len(dirs)} {prefix.rstrip('_')} sessions")

        for dir in dirs:
            accel_file = dir / "WatchAccelerometerUncalibrated.csv"
            hr_file = dir / "HeartRate.csv"

            if not accel_file.exists():
                continue

            accel_df = pd.read_csv(accel_file)

            if len(accel_df) < window_size:
                continue

            # Get accelerometer data
            features = accel_df[['x', 'y', 'z']].values

            # Add heart rate
            if hr_file.exists() and os.path.getsize(hr_file) > 50:
                hr_df = pd.read_csv(hr_file)
                if len(hr_df) > 0:
                    hr = hr_df['bpm'].mean()
                else:
                    hr = 100.0
            else:
                hr = 100.0

            hr_col = np.full((len(features), 1), hr)
            features = np.hstack([features, hr_col])

            # Split into 5-second windows
            num_windows = min(4, len(features) // window_size)
            for i in range(num_windows):
                start = i * window_size
                end = start + window_size
                if end <= len(features):
                    sequences.append(features[start:end])
                    labels.append(label)

    return sequences, labels

# Load data
print("Loading data...")
seqs, labs = load_workout_data()
print(f"Loaded {len(seqs)} sequences")

X = np.array(seqs)
y = np.array(labs)
print(f"Data shape: {X.shape}")

# Convert to tensors
X = torch.tensor(X, dtype=torch.float32)
y = torch.tensor(y, dtype=torch.long)

# Train/val split
X_train, X_val, y_train, y_val = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

workouts = ["WeightLift", "Walking", "Plank", "JumpingJacks", "Squats", "JumpRope"]
print(f"\nTrain: {len(X_train)}, Val: {len(X_val)}")

# Visualizations
colors = ['blue', 'orange', 'green', 'red', "purple", "black"]
feature_names = ["Accel X", "Accel Y", "Accel Z", "Heart Rate"]

fig, axes = plt.subplots(2, 2)
axes = axes.ravel()

for i, name in enumerate(feature_names):
    for c in range(len(workouts)):
        idx = np.where(y.numpy() == c)[0]
        data = X[idx][:, :, i].numpy()

        np.random.seed(42 + c)
        samples = np.random.choice(len(data), min(3, len(data)), replace=False)

        for s in samples:
            axes[i].plot(data[s], color=colors[c], alpha=0.3, linewidth=1)

        median = np.median(data, axis=0)
        axes[i].plot(median, label=workouts[c], color=colors[c], linewidth=2.5)

    axes[i].set_title(name, fontweight='bold')
    axes[i].legend(fontsize=8)
    axes[i].grid(alpha=0.3)

plt.tight_layout()
plt.show()

# FFT analysis
fig, axes = plt.subplots(2, 2)
axes = axes.ravel()

for i, name in enumerate(feature_names):
    for c in range(len(workouts)):
        idx = np.where(y.numpy() == c)[0]
        data = X[idx][:, :, i].numpy()

        ffts = []
        for sample in data:
            centered = sample - np.mean(sample)
            fft = np.abs(np.fft.fft(centered)[:len(centered)//2])
            ffts.append(fft)

        median_fft = np.median(ffts, axis=0)
        freqs = np.fft.fftfreq(X.shape[1], 1/100)[:X.shape[1]//2]

        axes[i].plot(freqs, median_fft, label=workouts[c],
                    color=colors[c], linewidth=2)

    axes[i].set_title(f'{name} - Frequency', fontweight='bold')
    axes[i].set_xlabel('Hz')
    axes[i].set_xlim([0, 10])
    axes[i].legend(fontsize=8)
    axes[i].grid(alpha=0.3)

plt.tight_layout()
plt.show()

# Model
class WorkoutLSTM(nn.Module):
    def __init__(self, input_size, hidden_size, num_classes):
        super().__init__()
        self.lstm = nn.LSTM(input_size, hidden_size, batch_first=True)
        self.fc = nn.Linear(hidden_size, num_classes)

    def forward(self, x):
        _, (h, _) = self.lstm(x)
        return self.fc(h[-1])

# Setup device
if torch.backends.mps.is_available():
    device = torch.device("mps")
elif torch.cuda.is_available():
    device = torch.device("cuda")
else:
    device = torch.device("cpu")

print(f"\nUsing {device}")

X_train = X_train.to(device)
y_train = y_train.to(device)
X_val = X_val.to(device)
y_val = y_val.to(device)

model = WorkoutLSTM(4, 64, len(workouts)).to(device)
criterion = nn.CrossEntropyLoss()
optimizer = torch.optim.AdamW(model.parameters(), lr=0.001, weight_decay=1e-4)
scheduler = torch.optim.lr_scheduler.ReduceLROnPlateau(
    optimizer, mode='min', factor=0.5, patience=10
)

# Training
print("\nTraining...")
epochs = 1000
train_losses, train_accs = [], []
val_losses, val_accs = [], []
best_val_acc = 0.0
best_model = None
patience = 0

start = time.time()

for epoch in range(epochs):
    model.train()
    optimizer.zero_grad(set_to_none=True)

    outputs = model(X_train)
    loss = criterion(outputs, y_train)
    loss.backward()
    nn.utils.clip_grad_norm_(model.parameters(), 1.0)
    optimizer.step()

    train_losses.append(loss.item())
    train_acc = (outputs.argmax(1) == y_train).float().mean().item()
    train_accs.append(train_acc)

    model.eval()
    with torch.no_grad():
        val_out = model(X_val)
        val_loss = criterion(val_out, y_val)
        val_acc = (val_out.argmax(1) == y_val).float().mean().item()
        val_losses.append(val_loss.item())
        val_accs.append(val_acc)

    scheduler.step(val_loss)

    if val_acc > best_val_acc:
        best_val_acc = val_acc
        best_model = model.state_dict().copy()
        patience = 0
    else:
        patience += 1

    if patience >= 150:
        print(f"Early stop at epoch {epoch+1}")
        break

    if (epoch + 1) % 10 == 0:
        print(f"Epoch {epoch+1}: Train {train_acc:.3f} | Val {val_acc:.3f} | Best {best_val_acc:.3f}")

elapsed = time.time() - start
print(f"\nTraining done in {elapsed:.1f}s")

if best_model:
    model.load_state_dict(best_model)
    print(f"Best val acc: {best_val_acc:.4f}")

# Evaluation
model = model.cpu()
X_train = X_train.cpu()
y_train = y_train.cpu()
X_val = X_val.cpu()
y_val = y_val.cpu()

model.eval()
with torch.no_grad():
    preds = model(X_val).argmax(1).numpy()

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

ax1.plot(train_losses, label='Train')
ax1.plot(val_losses, label='Val')
ax1.set_title("Loss")
ax1.legend()
ax1.grid(alpha=0.3)

ax2.plot(train_accs, label='Train')
ax2.plot(val_accs, label='Val')
ax2.set_title("Accuracy")
ax2.legend()
ax2.grid(alpha=0.3)

plt.tight_layout()
plt.show()

cm = confusion_matrix(y_val.numpy(), preds)
disp = ConfusionMatrixDisplay(cm, display_labels=workouts)
disp.plot(cmap='Blues', xticks_rotation=45)
plt.title("Validation Confusion Matrix")
plt.show()

print(f"\nValidation Results:")
for i, name in enumerate(workouts):
    total = (y_val.numpy() == i).sum()
    correct = ((y_val.numpy() == i) & (preds == i)).sum()
    print(f"  {name}: {correct}/{total} ({correct/total*100:.1f}%)")

# Export
import tempfile

def get_size(m):
    with tempfile.NamedTemporaryFile(delete=True) as f:
        torch.save(m.state_dict(), f.name)
        return os.path.getsize(f.name) / (1024 * 1024)

size_fp32 = get_size(model)
params = sum(p.numel() for p in model.parameters())

print(f"\nModel: {params:,} params, {size_fp32:.2f} MB")

# FP16
model_fp16 = WorkoutLSTM(4, 64, len(workouts))
model_fp16.load_state_dict(model.state_dict())
model_fp16 = model_fp16.half()

size_fp16 = get_size(model_fp16)

with torch.no_grad():
    preds_fp16 = model_fp16(X_val.half()).argmax(1).numpy()
    acc_fp16 = (preds_fp16 == y_val.numpy()).mean()

print(f"FP16: {size_fp16:.2f} MB ({size_fp16/size_fp32*100:.0f}%), acc {acc_fp16:.4f}")

# Compare confusion matrices
cm_fp16 = confusion_matrix(y_val.numpy(), preds_fp16)
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

disp_fp32 = ConfusionMatrixDisplay(cm, display_labels=workouts)
disp_fp32.plot(cmap='Blues', xticks_rotation=45, ax=ax1)
ax1.set_title("FP32 Model")

disp_fp16_cm = ConfusionMatrixDisplay(cm_fp16, display_labels=workouts)
disp_fp16_cm.plot(cmap='Greens', xticks_rotation=45, ax=ax2)
ax2.set_title("FP16 Model")

plt.tight_layout()
plt.show()

# Save
torch.save(model.state_dict(), 'workout_model.pth')
torch.save(model_fp16.state_dict(), 'workout_model_fp16.pth')
print("\nSaved models")
