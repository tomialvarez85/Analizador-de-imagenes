FROM python:3.11-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    default-libmysqlclient-dev \
    gcc \
    && rm -rf /var/lib/apt/lists/*

COPY app /app

RUN pip install --no-cache-dir \
    flask \
    flask-cors \
    mysql-connector-python \
    anthropic \
    python-dotenv \
    pyjwt

EXPOSE 8000

CMD ["python", "main.py"]
